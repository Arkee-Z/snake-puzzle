#include "snake_puzzle/core/game.h"

#include <stdio.h>
#include <string.h>

#include "snake_puzzle/core/score.h"

static bool game_fail(Game *game, const char *reason, int x, int y)
{
    if (game->shields > 0) {
        game->shields -= 1;
        event_push(&game->events, EVENT_SHIELD_SAVED, x, y, 0,
                   "Shield absorbed the hit: %s", reason);
        return true;
    }
    game->phase = GAME_DEAD;
    game->snake.alive = false;
    snprintf(game->death_reason, sizeof(game->death_reason), "%s", reason);
    event_push(&game->events, EVENT_DEATH, x, y, 0, "%s", reason);
    return false;
}

void game_reset_entities(Game *game)
{
    game->food_count = 0;
    game->item_count = 0;
    game->block_count = 0;
    game->gate_count = 0;
    game->plate_count = 0;
    game->teleport_count = 0;
    game->seal_count = 0;
    game->orb_count = 0;
    game->seals_collected = 0;
    game->orbs_collected = 0;
    game->pending_special_gate = -1;
    game->mini_exit_open = false;
    event_queue_init(&game->events);
}

void game_init(Game *game, int width, int height, uint64_t seed, GameKind kind)
{
    memset(game, 0, sizeof(*game));
    board_init(&game->board, width, height);
    game->kind = kind;
    game->phase = GAME_PLAYING;
    game->seed = seed;
    game->score = 0;
    game->moves = 0;
    game->foods_eaten = 0;
    game->keys = 0;
    game->shields = 0;
    game->phase_tokens = 0;
    game->double_next = false;
    game->death_reason[0] = '\0';
    rng_seed(&game->rng, rng_mix64(seed ^ (kind == GAME_SPECIAL ? UINT64_C(0x5150) : UINT64_C(0x51A7))));
    game_reset_entities(game);
}

bool game_set_cell(Game *game, int x, int y, Terrain terrain)
{
    Cell *cell = board_cell(&game->board, x, y);

    if (cell == NULL) {
        return false;
    }
    cell->type = terrain;
    cell->gate = -1;
    cell->plate = -1;
    cell->teleport = -1;
    cell->seal = -1;
    cell->orb = -1;
    cell->oneway_dir = DIR_NONE;
    cell->break_triggered = false;
    return true;
}

bool game_place_snake(Game *game, Point head, Direction dir, int length)
{
    int i;

    if (dir == DIR_NONE || length < 1 || length > SP_MAX_SNAKE) {
        return false;
    }
    snake_init(&game->snake, head, dir, length);
    for (i = 0; i < game->snake.length; ++i) {
        Point p = game->snake.body[i];
        const Cell *cell = board_cell_const(&game->board, p.x, p.y);
        if (cell == NULL || cell->type == TERRAIN_WALL) {
            snake_clear(&game->snake);
            return false;
        }
    }
    return true;
}

int game_add_food(Game *game, int x, int y, FoodKind kind)
{
    int index;

    if (game->food_count >= SP_MAX_FOODS || board_cell(&game->board, x, y) == NULL) {
        return -1;
    }
    index = game->food_count++;
    game->foods[index].x = x;
    game->foods[index].y = y;
    game->foods[index].kind = kind;
    game->foods[index].active = true;
    return index;
}

int game_add_item(Game *game, int x, int y, ItemType type)
{
    int index;

    if (game->item_count >= SP_MAX_ITEMS || type < 0 || type >= ITEM_COUNT ||
        board_cell(&game->board, x, y) == NULL) {
        return -1;
    }
    index = game->item_count++;
    game->items[index].x = x;
    game->items[index].y = y;
    game->items[index].type = type;
    game->items[index].active = true;
    return index;
}

int game_add_block(Game *game, int x, int y)
{
    int index;

    if (game->block_count >= SP_MAX_BLOCKS || board_cell(&game->board, x, y) == NULL) {
        return -1;
    }
    index = game->block_count++;
    game->blocks[index].x = x;
    game->blocks[index].y = y;
    game->blocks[index].active = true;
    return index;
}

int game_add_gate(Game *game, GateKind kind, int x, int y, int required_seals,
                  int required_orbs)
{
    int index;
    Cell *cell;
    Terrain terrain;

    if (game->gate_count >= SP_MAX_GATES) {
        return -1;
    }
    index = game->gate_count++;
    game->gates[index].kind = kind;
    game->gates[index].x = x;
    game->gates[index].y = y;
    game->gates[index].open = false;
    game->gates[index].used = false;
    game->gates[index].required_seals = required_seals;
    game->gates[index].required_orbs = required_orbs;

    terrain = TERRAIN_LOCKED;
    if (kind == GATE_SPECIAL) {
        terrain = TERRAIN_SPECIAL_GATE;
    } else if (kind == GATE_MINI_EXIT) {
        terrain = TERRAIN_MINI_EXIT;
    }
    cell = board_cell(&game->board, x, y);
    if (cell == NULL) {
        game->gate_count -= 1;
        return -1;
    }
    cell->type = terrain;
    cell->gate = (int8_t)index;
    return index;
}

int game_add_plate(Game *game, int x, int y, uint64_t gate_mask)
{
    int index;
    Cell *cell;

    if (game->plate_count >= SP_MAX_PLATES) {
        return -1;
    }
    cell = board_cell(&game->board, x, y);
    if (cell == NULL) {
        return -1;
    }
    index = game->plate_count++;
    game->plates[index].x = x;
    game->plates[index].y = y;
    game->plates[index].pressed = false;
    game->plates[index].gate_mask = gate_mask;
    cell->type = TERRAIN_PLATE;
    cell->plate = (int8_t)index;
    return index;
}

int game_add_teleport(Game *game, int x1, int y1, int x2, int y2)
{
    int index;
    Cell *a;
    Cell *b;

    if (game->teleport_count >= SP_MAX_TELEPORTS) {
        return -1;
    }
    a = board_cell(&game->board, x1, y1);
    b = board_cell(&game->board, x2, y2);
    if (a == NULL || b == NULL || (x1 == x2 && y1 == y2)) {
        return -1;
    }
    index = game->teleport_count++;
    game->teleports[index].x1 = x1;
    game->teleports[index].y1 = y1;
    game->teleports[index].x2 = x2;
    game->teleports[index].y2 = y2;
    game->teleports[index].active = true;
    a->type = TERRAIN_TELEPORT;
    a->teleport = (int8_t)index;
    b->type = TERRAIN_TELEPORT;
    b->teleport = (int8_t)index;
    return index;
}

int game_add_seal(Game *game, int x, int y)
{
    int index;
    Cell *cell;

    if (game->seal_count >= SP_MAX_SEALS) {
        return -1;
    }
    cell = board_cell(&game->board, x, y);
    if (cell == NULL) {
        return -1;
    }
    index = game->seal_count++;
    game->seals[index].x = x;
    game->seals[index].y = y;
    game->seals[index].collected = false;
    cell->type = TERRAIN_SEAL;
    cell->seal = (int8_t)index;
    return index;
}

int game_add_orb(Game *game, int x, int y)
{
    int index;
    Cell *cell;

    if (game->orb_count >= SP_MAX_ORBS) {
        return -1;
    }
    cell = board_cell(&game->board, x, y);
    if (cell == NULL) {
        return -1;
    }
    index = game->orb_count++;
    game->orbs[index].x = x;
    game->orbs[index].y = y;
    game->orbs[index].collected = false;
    cell->type = TERRAIN_ORB;
    cell->orb = (int8_t)index;
    return index;
}

Food *game_food_at(Game *game, int x, int y)
{
    int i;

    for (i = 0; i < game->food_count; ++i) {
        if (game->foods[i].active && game->foods[i].x == x && game->foods[i].y == y) {
            return &game->foods[i];
        }
    }
    return NULL;
}

Item *game_item_at(Game *game, int x, int y)
{
    int i;

    for (i = 0; i < game->item_count; ++i) {
        if (game->items[i].active && game->items[i].x == x && game->items[i].y == y) {
            return &game->items[i];
        }
    }
    return NULL;
}

Block *game_block_at(Game *game, int x, int y)
{
    int i;

    for (i = 0; i < game->block_count; ++i) {
        if (game->blocks[i].active && game->blocks[i].x == x && game->blocks[i].y == y) {
            return &game->blocks[i];
        }
    }
    return NULL;
}

Gate *game_gate_at(Game *game, int x, int y)
{
    int i;

    for (i = 0; i < game->gate_count; ++i) {
        if (game->gates[i].x == x && game->gates[i].y == y) {
            return &game->gates[i];
        }
    }
    return NULL;
}

void game_open_gate(Game *game, int gate_index, const char *reason)
{
    Gate *gate;

    if (gate_index < 0 || gate_index >= game->gate_count) {
        return;
    }
    gate = &game->gates[gate_index];
    if (gate->open) {
        return;
    }
    gate->open = true;
    event_push(&game->events, EVENT_GATE_OPENED, gate->x, gate->y, 0, "%s", reason);
    if (gate->kind == GATE_SPECIAL) {
        event_push(&game->events, EVENT_SPECIAL_UNLOCKED, gate->x, gate->y, 0,
                   "The special gate awakens. Its seal is broken.");
    }
}

void game_press_plate(Game *game, int plate_index)
{
    Plate *plate;
    int i;

    if (plate_index < 0 || plate_index >= game->plate_count) {
        return;
    }
    plate = &game->plates[plate_index];
    if (plate->pressed) {
        return;
    }
    plate->pressed = true;
    event_push(&game->events, EVENT_PLATE_PRESSED, plate->x, plate->y, 0,
               "A pressure plate locks into place.");
    for (i = 0; i < game->gate_count && i < 64; ++i) {
        if ((plate->gate_mask & (UINT64_C(1) << i)) != 0U && !game->gates[i].open) {
            game_open_gate(game, i, "A distant mechanism unlocks a door.");
        }
    }
}

void game_refresh_gates(Game *game)
{
    int i;

    for (i = 0; i < game->gate_count; ++i) {
        Gate *gate = &game->gates[i];

        if (gate->used || gate->open) {
            continue;
        }
        if (gate->kind == GATE_SPECIAL && game->seals_collected >= gate->required_seals) {
            game_open_gate(game, i, "The final seal resonates. The way opens.");
        } else if (gate->kind == GATE_MINI_EXIT &&
                   game->orbs_collected >= gate->required_orbs) {
            gate->open = true;
            game->mini_exit_open = true;
            event_push(&game->events, EVENT_MINI_UNLOCKED, gate->x, gate->y, 0,
                       "The chamber exit unfolds.");
        }
    }
}

void game_apply_item(Game *game, ItemType type)
{
    switch (type) {
    case ITEM_KEY:
        game->keys += 1;
        break;
    case ITEM_SHIELD:
        game->shields += 1;
        break;
    case ITEM_PHASE:
        game->phase_tokens += 1;
        break;
    case ITEM_DOUBLE:
        game->double_next = true;
        break;
    default:
        break;
    }
}

bool game_can_spawn_at(const Game *game, int x, int y)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    int i;

    if (cell == NULL || cell->type != TERRAIN_FLOOR) {
        return false;
    }
    if (snake_occupies(&game->snake, x, y)) {
        return false;
    }
    for (i = 0; i < game->block_count; ++i) {
        if (game->blocks[i].active && game->blocks[i].x == x && game->blocks[i].y == y) {
            return false;
        }
    }
    for (i = 0; i < game->food_count; ++i) {
        if (game->foods[i].active && game->foods[i].x == x && game->foods[i].y == y) {
            return false;
        }
    }
    for (i = 0; i < game->item_count; ++i) {
        if (game->items[i].active && game->items[i].x == x && game->items[i].y == y) {
            return false;
        }
    }
    return true;
}

void game_spawn_food(Game *game, FoodKind kind)
{
    int candidates[SP_MAX_WIDTH * SP_MAX_HEIGHT];
    int count = 0;
    int x;
    int y;

    if (game->kind == GAME_SPECIAL || game->food_count >= SP_MAX_FOODS) {
        return;
    }
    for (y = 0; y < game->board.height; ++y) {
        for (x = 0; x < game->board.width; ++x) {
            if (game_can_spawn_at(game, x, y)) {
                candidates[count++] = y * game->board.width + x;
            }
        }
    }
    if (count == 0) {
        return;
    }
    {
        int pick = candidates[rng_range(&game->rng, (uint32_t)count)];
        game_add_food(game, pick % game->board.width, pick / game->board.width, kind);
    }
}

static bool teleport_can_land(const Game *game, int x, int y)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    int i;

    if (cell == NULL) {
        return false;
    }
    if (cell->type != TERRAIN_FLOOR && cell->type != TERRAIN_PLATE &&
        cell->type != TERRAIN_SEAL && cell->type != TERRAIN_ORB &&
        cell->type != TERRAIN_BREAK && cell->type != TERRAIN_TELEPORT) {
        return false;
    }
    if (snake_occupies(&game->snake, x, y)) {
        return false;
    }
    for (i = 0; i < game->block_count; ++i) {
        if (game->blocks[i].active && game->blocks[i].x == x && game->blocks[i].y == y) {
            return false;
        }
    }
    return true;
}

static bool block_can_enter(const Game *game, int x, int y, bool *is_plate)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    int i;

    *is_plate = false;
    if (cell == NULL) {
        return false;
    }
    if (cell->type == TERRAIN_PLATE) {
        *is_plate = true;
    } else if (cell->type != TERRAIN_FLOOR) {
        return false;
    }
    if (snake_occupies(&game->snake, x, y)) {
        return false;
    }
    for (i = 0; i < game->block_count; ++i) {
        if (game->blocks[i].active && game->blocks[i].x == x && game->blocks[i].y == y) {
            return false;
        }
    }
    for (i = 0; i < game->food_count; ++i) {
        if (game->foods[i].active && game->foods[i].x == x && game->foods[i].y == y) {
            return false;
        }
    }
    for (i = 0; i < game->item_count; ++i) {
        if (game->items[i].active && game->items[i].x == x && game->items[i].y == y) {
            return false;
        }
    }
    return true;
}

static bool slide_block(Game *game, Block *block, Direction dir)
{
    int x = block->x;
    int y = block->y;
    int nx;
    int ny;
    bool moved = false;
    bool is_plate = false;

    for (;;) {
        bool next_plate = false;

        nx = x + dir_dx(dir);
        ny = y + dir_dy(dir);
        if (!block_can_enter(game, nx, ny, &next_plate)) {
            break;
        }
        x = nx;
        y = ny;
        moved = true;
        is_plate = next_plate;
        if (next_plate) {
            break;
        }
    }
    if (!moved) {
        return false;
    }
    block->x = x;
    block->y = y;
    event_push(&game->events, EVENT_PUSH_BLOCK, x, y, 0,
               "The stone grinds to a halt.");
    if (is_plate) {
        Cell *cell = board_cell(&game->board, x, y);
        if (cell != NULL && cell->type == TERRAIN_PLATE && cell->plate >= 0) {
            game_press_plate(game, cell->plate);
        }
    }
    return true;
}

static void collapse_break_floors(Game *game)
{
    int x;
    int y;

    for (y = 0; y < game->board.height; ++y) {
        for (x = 0; x < game->board.width; ++x) {
            Cell *cell = board_cell(&game->board, x, y);
            if (cell->type == TERRAIN_BREAK && cell->break_triggered &&
                !snake_occupies(&game->snake, x, y)) {
                cell->type = TERRAIN_PIT;
                event_push(&game->events, EVENT_BREAK_FLOOR, x, y, 0,
                           "The fractured floor gives way behind you.");
            }
        }
    }
}

static StepResult game_enter_fatal(Game *game, const char *reason, int x, int y)
{
    return game_fail(game, reason, x, y) ? STEP_OK : STEP_DIED;
}

StepResult game_step(Game *game, Direction dir)
{
    Point target;
    Point final_point;
    Cell *cell;
    bool will_grow;
    bool teleported = false;
    bool entered_special = false;
    bool completed_mini = false;
    int i;

    if (game == NULL || game->phase != GAME_PLAYING) {
        return STEP_INVALID;
    }
    if (dir == DIR_NONE || game->snake.length <= 0) {
        return STEP_INVALID;
    }
    if (game->pending_special_gate >= 0) {
        return STEP_INVALID;
    }
    if (dir == dir_opposite(game->snake.dir)) {
        return game_enter_fatal(game, "You reversed into your own neck.", game->snake.body[0].x,
                                game->snake.body[0].y);
    }

    target.x = game->snake.body[0].x + dir_dx(dir);
    target.y = game->snake.body[0].y + dir_dy(dir);
    if (!board_in_bounds(&game->board, target.x, target.y)) {
        return game_enter_fatal(game, "You struck the edge of the world.", target.x, target.y);
    }
    cell = board_cell(&game->board, target.x, target.y);
    final_point = target;

    if (cell->type == TERRAIN_WALL) {
        if (game->phase_tokens > 0) {
            game->phase_tokens -= 1;
            cell->type = TERRAIN_FLOOR;
            cell->gate = -1;
            event_push(&game->events, EVENT_INFO, target.x, target.y, 0,
                       "The phase charge dissolves a wall.");
        } else {
            return game_enter_fatal(game, "You hit a wall.", target.x, target.y);
        }
    }

    if (cell->type == TERRAIN_ONEWAY) {
        if ((Direction)cell->oneway_dir != dir) {
            return game_enter_fatal(game, "The one-way gate rejects that approach.", target.x,
                                    target.y);
        }
    }

    if (cell->type == TERRAIN_LOCKED) {
        if (cell->gate < 0 || cell->gate >= game->gate_count) {
            return game_enter_fatal(game, "The door mechanism is broken.", target.x, target.y);
        }
        if (!game->gates[cell->gate].open) {
            if (game->keys <= 0) {
                return game_enter_fatal(game, "The door is locked and you have no key.",
                                        target.x, target.y);
            }
            game->keys -= 1;
            game_open_gate(game, cell->gate, "The key turns. The lock falls away.");
        }
    }

    if (cell->type == TERRAIN_SPECIAL_GATE) {
        if (cell->gate < 0 || cell->gate >= game->gate_count ||
            !game->gates[cell->gate].open || game->gates[cell->gate].used) {
            return game_enter_fatal(game, "The special gate is sealed.", target.x, target.y);
        }
        entered_special = true;
    }

    if (cell->type == TERRAIN_MINI_EXIT) {
        if (cell->gate < 0 || cell->gate >= game->gate_count ||
            !game->gates[cell->gate].open) {
            return game_enter_fatal(game, "The chamber exit is still sealed.", target.x,
                                    target.y);
        }
        completed_mini = true;
    }

    {
        Block *block = game_block_at(game, target.x, target.y);
        if (block != NULL) {
            if (!slide_block(game, block, dir)) {
                return game_enter_fatal(game, "The stone will not budge.", target.x, target.y);
            }
            /* The head now moves into the cell the stone vacated. */
            cell = board_cell(&game->board, target.x, target.y);
        }
    }

    if (cell->type == TERRAIN_PIT) {
        return game_enter_fatal(game, "The floor has already collapsed there.", target.x,
                                target.y);
    }

    if (cell->type == TERRAIN_TELEPORT) {
        int teleport_index = cell->teleport;
        TeleportPair *pair;
        int dest_x;
        int dest_y;

        if (teleport_index < 0 || teleport_index >= game->teleport_count) {
            return game_enter_fatal(game, "The teleporter is unstable.", target.x, target.y);
        }
        pair = &game->teleports[teleport_index];
        if (pair->x1 == target.x && pair->y1 == target.y) {
            dest_x = pair->x2;
            dest_y = pair->y2;
        } else {
            dest_x = pair->x1;
            dest_y = pair->y1;
        }
        if (!teleport_can_land(game, dest_x, dest_y)) {
            return game_enter_fatal(game, "The teleporter discharge point is blocked.", dest_x,
                                    dest_y);
        }
        final_point.x = dest_x;
        final_point.y = dest_y;
        teleported = true;
    }

    will_grow = game_food_at(game, final_point.x, final_point.y) != NULL;
    for (i = 0; i < game->snake.length; ++i) {
        if (game->snake.body[i].x == final_point.x &&
            game->snake.body[i].y == final_point.y) {
            if (i == game->snake.length - 1 && !will_grow) {
                continue;
            }
            return game_enter_fatal(game, "You bit into your own body.", final_point.x,
                                    final_point.y);
        }
    }

    if (!snake_move(&game->snake, final_point, will_grow)) {
        return STEP_INVALID;
    }
    game->snake.dir = dir;
    game->moves += 1;
    event_push(&game->events, EVENT_MOVE, final_point.x, final_point.y, 0, NULL);
    if (teleported) {
        event_push(&game->events, EVENT_TELEPORT, target.x, target.y, 0,
                   "Space folds. You emerge elsewhere.");
    }

    cell = board_cell(&game->board, final_point.x, final_point.y);
    if (game_food_at(game, final_point.x, final_point.y) != NULL) {
        Food *food = game_food_at(game, final_point.x, final_point.y);
        FoodKind kind = food->kind;
        int gain = food_base_score(kind);

        if (game->double_next) {
            gain *= 2;
            game->double_next = false;
        }
        food->active = false;
        game->score += gain;
        game->foods_eaten += 1;
        event_push(&game->events, EVENT_EAT_FOOD, final_point.x, final_point.y, gain,
                   "%s devoured. +%d", kind == FOOD_GOLDEN ? "Golden fruit" : "Fruit", gain);
        game_spawn_food(game, kind);
    }

    if (game_item_at(game, final_point.x, final_point.y) != NULL) {
        Item *item = game_item_at(game, final_point.x, final_point.y);
        ItemType type = item->type;
        const ItemDef *def = item_def(type);

        item->active = false;
        game->score += def->score;
        game_apply_item(game, type);
        event_push(&game->events, EVENT_PICKUP_ITEM, final_point.x, final_point.y, def->score,
                   "Picked up %s. %s", def->name, def->description);
    }

    if (cell->type == TERRAIN_SEAL && cell->seal >= 0 && cell->seal < game->seal_count) {
        Seal *seal = &game->seals[cell->seal];
        if (!seal->collected) {
            int gain = SP_SCORE_SEAL_BASE + SP_SCORE_SEAL_STEP * game->seals_collected;
            seal->collected = true;
            game->seals_collected += 1;
            game->score += gain;
            event_push(&game->events, EVENT_SEAL_COLLECTED, final_point.x, final_point.y, gain,
                       "Seal %d/%d secured. +%d", game->seals_collected, game->seal_count,
                       gain);
            game_refresh_gates(game);
        }
    }

    if (cell->type == TERRAIN_ORB && cell->orb >= 0 && cell->orb < game->orb_count) {
        Orb *orb = &game->orbs[cell->orb];
        if (!orb->collected) {
            orb->collected = true;
            game->orbs_collected += 1;
            game->score += SP_SCORE_ORB;
            event_push(&game->events, EVENT_ORB_COLLECTED, final_point.x, final_point.y,
                       SP_SCORE_ORB, "Chamber orb %d/%d. +%d", game->orbs_collected,
                       game->orb_count, SP_SCORE_ORB);
            game_refresh_gates(game);
        }
    }

    if (cell->type == TERRAIN_PLATE && cell->plate >= 0) {
        game_press_plate(game, cell->plate);
    }

    if (cell->type == TERRAIN_BREAK) {
        cell->break_triggered = true;
        event_push(&game->events, EVENT_BREAK_FLOOR, final_point.x, final_point.y, 0,
                   "The fractured floor cracks under your weight.");
    }

    collapse_break_floors(game);

    if (entered_special) {
        Gate *gate = &game->gates[cell->gate];
        game->pending_special_gate = cell->gate;
        event_push(&game->events, EVENT_ENTER_SPECIAL, final_point.x, final_point.y, 0,
                   "You slip through the special gate.");
        SP_UNUSED(gate);
        return STEP_SPECIAL_ENTERED;
    }

    if (completed_mini) {
        game->phase = GAME_MINI_COMPLETE;
        event_push(&game->events, EVENT_SPECIAL_FINISHED, final_point.x, final_point.y, 0,
                   "The chamber yields its prize.");
        return STEP_MINI_COMPLETE;
    }

    return STEP_OK;
}

int game_special_bonus(const Game *chamber)
{
    return SP_SCORE_SPECIAL_BASE + SP_SCORE_ORB * chamber->orb_count;
}

bool game_finish_special(Game *main_game, int bonus)
{
    Gate *gate;
    Cell *cell;

    if (main_game->pending_special_gate < 0 ||
        main_game->pending_special_gate >= main_game->gate_count) {
        return false;
    }
    gate = &main_game->gates[main_game->pending_special_gate];
    gate->used = true;
    gate->open = false;
    cell = board_cell(&main_game->board, gate->x, gate->y);
    if (cell != NULL) {
        cell->type = TERRAIN_FLOOR;
        cell->gate = -1;
    }
    main_game->score += bonus;
    event_push(&main_game->events, EVENT_SPECIAL_FINISHED, gate->x, gate->y, bonus,
               "Chamber cleared. Returned with +%d score.", bonus);
    main_game->pending_special_gate = -1;
    return true;
}
