#include "sp.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const int DX[4] = {0, 1, 0, -1};
static const int DY[4] = {-1, 0, 1, 0};

static int clamp_int(int value, int low, int high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static uint64_t sp_rng_next(uint64_t *state)
{
    uint64_t z;

    *state += UINT64_C(0x9E3779B97F4A7C15);
    z = *state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static int rng_range(uint64_t *state, int bound)
{
    if (bound <= 1) {
        return 0;
    }
    return (int)(sp_rng_next(state) % (uint64_t)bound);
}

static void set_message(SpGame *game, const char *text)
{
    snprintf(game->message, sizeof(game->message), "%s", text != NULL ? text : "");
}

const char *sp_dir_name(SpDir dir)
{
    switch (dir) {
    case SP_DIR_UP:
        return "up";
    case SP_DIR_RIGHT:
        return "right";
    case SP_DIR_DOWN:
        return "down";
    case SP_DIR_LEFT:
        return "left";
    default:
        return "none";
    }
}

SpDir sp_dir_from_char(char c)
{
    switch (c) {
    case 'w':
    case 'W':
        return SP_DIR_UP;
    case 'd':
    case 'D':
        return SP_DIR_RIGHT;
    case 's':
    case 'S':
        return SP_DIR_DOWN;
    case 'a':
    case 'A':
        return SP_DIR_LEFT;
    default:
        return SP_DIR_NONE;
    }
}

SpDir sp_dir_opposite(SpDir dir)
{
    switch (dir) {
    case SP_DIR_UP:
        return SP_DIR_DOWN;
    case SP_DIR_RIGHT:
        return SP_DIR_LEFT;
    case SP_DIR_DOWN:
        return SP_DIR_UP;
    case SP_DIR_LEFT:
        return SP_DIR_RIGHT;
    default:
        return SP_DIR_NONE;
    }
}

static bool in_bounds(const SpGame *game, int x, int y)
{
    return x >= 0 && y >= 0 && x < game->width && y < game->height;
}

static unsigned char cell_at(const SpGame *game, int x, int y)
{
    if (!in_bounds(game, x, y)) {
        return SP_CELL_WALL;
    }
    return game->cells[y][x];
}

static bool body_at(const SpGame *game, int x, int y, int ignore_index)
{
    int i;

    for (i = 0; i < game->length; ++i) {
        if (i != ignore_index && game->snake[i].x == x && game->snake[i].y == y) {
            return true;
        }
    }
    return false;
}

static bool body_blocks(const SpGame *game, int x, int y)
{
    int tail = game->length > 0 ? game->length - 1 : -1;
    return body_at(game, x, y, tail);
}

static void set_quota(SpGame *game)
{
    int total = game->turn_food_total;

    game->turn_food_min = total > 1 ? total / 2 : 1;
    if (game->turn_food_min < 1) {
        game->turn_food_min = 1;
    }
    game->turn_food_max = total > 2 ? total - 1 : total;
    if (game->turn_food_max < game->turn_food_min) {
        game->turn_food_max = game->turn_food_min;
    }
}

void game_new(SpGame *game)
{
    memset(game, 0, sizeof(*game));
    game->width = 20;
    game->height = 15;
    game->mode = SP_MODE_TURN;
    game->dir = SP_DIR_UP;
    game->door_x = -1;
    game->door_y = -1;
    game->exit_x = -1;
    game->exit_y = -1;
    set_message(game, "New run.");
}

void game_start(SpGame *game, uint64_t now_ms)
{
    game->start_ms = now_ms;
    game->end_ms = 0;
}

static bool place_snake_dir(SpGame *game, int head_x, int head_y, SpDir dir, int length)
{
    int i;

    if (length < 1 || length > SP_SNAKE_MAX) {
        return false;
    }
    for (i = 0; i < length; ++i) {
        int x = head_x - DX[dir] * i;
        int y = head_y - DY[dir] * i;
        unsigned char cell;
        if (!in_bounds(game, x, y)) {
            return false;
        }
        cell = cell_at(game, x, y);
        if (cell == SP_CELL_WALL || cell == SP_CELL_DOOR || cell == SP_CELL_EXIT) {
            return false;
        }
    }
    game->length = length;
    game->dir = dir;
    for (i = 0; i < length; ++i) {
        game->snake[i].x = head_x - DX[dir] * i;
        game->snake[i].y = head_y - DY[dir] * i;
    }
    if (game->max_length < length) {
        game->max_length = length;
    }
    return true;
}

static bool place_initial_snake(SpGame *game, int head_x, int head_y)
{
    static const SpDir ORDER[4] = {SP_DIR_UP, SP_DIR_RIGHT, SP_DIR_DOWN, SP_DIR_LEFT};
    int i;

    for (i = 0; i < 4; ++i) {
        if (place_snake_dir(game, head_x, head_y, ORDER[i], 3)) {
            return true;
        }
    }
    return false;
}

static void clear_zone(SpGame *game, int cx, int cy, int radius)
{
    int y;
    int x;

    for (y = cy - radius; y <= cy + radius; ++y) {
        for (x = cx - radius; x <= cx + radius; ++x) {
            if (!in_bounds(game, x, y) || x == 0 || y == 0 || x == game->width - 1 ||
                y == game->height - 1) {
                continue;
            }
            if (game->cells[y][x] != SP_CELL_DOOR && game->cells[y][x] != SP_CELL_EXIT) {
                game->cells[y][x] = SP_CELL_EMPTY;
            }
        }
    }
}

static void fill_base_map(SpGame *game, int width, int height)
{
    int x;
    int y;

    game->width = clamp_int(width, 5, SP_MAP_W);
    game->height = clamp_int(height, 5, SP_MAP_H);
    for (y = 0; y < game->height; ++y) {
        for (x = 0; x < game->width; ++x) {
            bool border = x == 0 || y == 0 || x == game->width - 1 || y == game->height - 1;
            game->cells[y][x] = border ? SP_CELL_WALL : SP_CELL_EMPTY;
        }
    }
}

static void add_random_walls(SpGame *game, uint64_t *rng, int attempts, int avoid_x1,
                             int avoid_y1, int avoid_x2, int avoid_y2)
{
    int i;

    for (i = 0; i < attempts; ++i) {
        int x = 1 + rng_range(rng, game->width - 2);
        int y = 1 + rng_range(rng, game->height - 2);
        if (x >= avoid_x1 && x <= avoid_x2 && y >= avoid_y1 && y <= avoid_y2) {
            continue;
        }
        if (game->cells[y][x] != SP_CELL_EMPTY) {
            continue;
        }
        game->cells[y][x] = SP_CELL_WALL;
        if (rng_range(rng, 100) < 35) {
            int nx = x + (rng_range(rng, 2) == 0 ? -1 : 1);
            int ny = y + (rng_range(rng, 2) == 0 ? -1 : 1);
            if (in_bounds(game, nx, ny) && game->cells[ny][nx] == SP_CELL_EMPTY &&
                !(nx >= avoid_x1 && nx <= avoid_x2 && ny >= avoid_y1 && ny <= avoid_y2)) {
                game->cells[ny][nx] = SP_CELL_WALL;
            }
        }
    }
}

static int collect_reachable(const SpGame *game, SpPos cells[], int max_cells, bool avoid_body)
{
    unsigned char seen[SP_MAP_H][SP_MAP_W];
    SpPos queue[SP_MAP_W * SP_MAP_H];
    int head = 0;
    int tail = 0;
    int count = 0;
    int d;

    memset(seen, 0, sizeof(seen));
    if (game->length < 1) {
        return 0;
    }
    queue[tail++] = game->snake[0];
    seen[game->snake[0].y][game->snake[0].x] = 1;
    while (head < tail) {
        SpPos cur = queue[head++];
        if (count < max_cells) {
            cells[count++] = cur;
        }
        for (d = 0; d < 4; ++d) {
            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            unsigned char cell;
            if (!in_bounds(game, nx, ny) || seen[ny][nx]) {
                continue;
            }
            cell = game->cells[ny][nx];
            if (cell == SP_CELL_WALL) {
                continue;
            }
            if (avoid_body && body_blocks(game, nx, ny)) {
                continue;
            }
            seen[ny][nx] = 1;
            queue[tail].x = nx;
            queue[tail].y = ny;
            tail++;
        }
    }
    return count;
}

static bool reachable_contains(const SpGame *game, int x, int y)
{
    SpPos cells[SP_MAP_W * SP_MAP_H];
    int count = collect_reachable(game, cells, SP_MAP_W * SP_MAP_H, true);
    int i;

    for (i = 0; i < count; ++i) {
        if (cells[i].x == x && cells[i].y == y) {
            return true;
        }
    }
    return false;
}

static int place_food(SpGame *game, uint64_t *rng, int food_count)
{
    SpPos cells[SP_MAP_W * SP_MAP_H];
    int count = collect_reachable(game, cells, SP_MAP_W * SP_MAP_H, true);
    int placed = 0;
    int guard = 0;

    if (count <= 0) {
        return 0;
    }
    while (placed < food_count && guard < 1000) {
        int index = rng_range(rng, count);
        SpPos p = cells[index];
        ++guard;
        if (game->cells[p.y][p.x] != SP_CELL_EMPTY) {
            continue;
        }
        if (body_at(game, p.x, p.y, -1)) {
            continue;
        }
        game->cells[p.y][p.x] = SP_CELL_FOOD;
        placed++;
    }
    return placed;
}

static void reset_run_fields(SpGame *game)
{
    game->mode = SP_MODE_TURN;
    game->turn_food_eaten = 0;
    game->total_food_eaten = 0;
    game->max_length = game->length;
    game->phase_bonus = 0;
    game->moves = 0;
    game->realtime_ticks = 0;
    game->start_ms = 0;
    game->end_ms = 0;
    game->debug = false;
    game->last_key = 0;
}

static bool autoplay_turn_solves(const SpGame *source)
{
    SpGame probe = *source;
    int i;

    for (i = 0; i < 800; ++i) {
        SpDir dir;
        SpStep step;
        if (probe.mode != SP_MODE_TURN) {
            return false;
        }
        dir = bfs_next_direction(&probe);
        if (dir == SP_DIR_NONE) {
            return false;
        }
        step = game_step(&probe, dir);
        if (step == SP_STEP_DOOR) {
            return true;
        }
        if (step != SP_STEP_OK) {
            return false;
        }
    }
    return false;
}

static bool autoplay_realtime_solves(const SpGame *source)
{
    SpGame probe = *source;
    int i;

    for (i = 0; i < 800; ++i) {
        SpDir dir;
        SpStep step;
        if (probe.mode != SP_MODE_REALTIME) {
            return false;
        }
        dir = bfs_next_direction(&probe);
        if (dir == SP_DIR_NONE) {
            return false;
        }
        step = game_step(&probe, dir);
        if (step == SP_STEP_EXIT) {
            return true;
        }
        if (step != SP_STEP_OK) {
            return false;
        }
    }
    return false;
}

bool game_generate_turn(SpGame *game, int width, int height, uint64_t seed)
{
    int attempt;

    if (width < 12 || width > SP_MAP_W || height < 8 || height > SP_MAP_H) {
        return false;
    }
    for (attempt = 0; attempt < 96; ++attempt) {
        uint64_t rng = seed + (uint64_t)attempt * UINT64_C(0x9E3779B97F4A7C15);
        int start_x = 2;
        int start_y = 2;
        int door_x;
        int door_y;
        int food_count;
        int reachable_count;
        SpPos cells[SP_MAP_W * SP_MAP_H];

        game_new(game);
        fill_base_map(game, width, height);
        door_x = game->width - 2;
        door_y = game->height - 2;
        game->cells[door_y][door_x] = SP_CELL_DOOR;
        game->door_x = door_x;
        game->door_y = door_y;

        clear_zone(game, start_x, start_y, 3);
        clear_zone(game, door_x, door_y, 2);
        add_random_walls(game, &rng, (game->width * game->height) / 6, 1, 1, 6, 6);
        clear_zone(game, start_x, start_y, 3);
        clear_zone(game, door_x, door_y, 2);
        game->cells[door_y][door_x] = SP_CELL_DOOR;

        if (!place_initial_snake(game, start_x, start_y)) {
            continue;
        }
        if (!reachable_contains(game, door_x, door_y)) {
            continue;
        }
        food_count = game->width >= 18 && game->height >= 12 ? 6 : 4;
        if (place_food(game, &rng, food_count) != food_count) {
            continue;
        }
        reachable_count = collect_reachable(game, cells, SP_MAP_W * SP_MAP_H, true);
        if (reachable_count < food_count + 3) {
            continue;
        }
        game->turn_food_total = food_count;
        game->seed = seed;
        reset_run_fields(game);
        set_quota(game);
        if (!autoplay_turn_solves(game)) {
            continue;
        }
        snprintf(game->message, sizeof(game->message),
                 "Random map. Eat %d-%d food, then enter the door.", game->turn_food_min,
                 game->turn_food_max);
        return true;
    }
    return false;
}

bool game_load_map(SpGame *game, const char *path, char *error, size_t error_size)
{
    FILE *file;
    char rows[SP_MAP_H][SP_MAP_W + 3];
    char line[SP_MAP_W + 3];
    int height = 0;
    int width = 0;
    int line_no = 0;
    int start_x = -1;
    int start_y = -1;
    int door_count = 0;
    int exit_count = 0;
    int head_count = 0;
    int food_count = 0;
    int y;
    int x;

    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "cannot open map: %s", path);
        return false;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len;
        ++line_no;
        len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0 || line[0] == ';') {
            continue;
        }
        if (len > (size_t)SP_MAP_W || height >= SP_MAP_H) {
            fclose(file);
            snprintf(error, error_size, "%s:%d: map exceeds %dx%d", path, line_no,
                     SP_MAP_W, SP_MAP_H);
            return false;
        }
        if (width == 0) {
            width = (int)len;
        }
        if ((int)len != width) {
            fclose(file);
            snprintf(error, error_size, "%s:%d: inconsistent row width", path, line_no);
            return false;
        }
        memcpy(rows[height], line, len + 1);
        height++;
    }
    fclose(file);
    if (height == 0 || width == 0) {
        snprintf(error, error_size, "%s: map is empty", path);
        return false;
    }

    game_new(game);
    fill_base_map(game, width, height);
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            char c = rows[y][x];
            switch (c) {
            case '#':
                game->cells[y][x] = SP_CELL_WALL;
                break;
            case '.':
                game->cells[y][x] = SP_CELL_EMPTY;
                break;
            case '*':
                game->cells[y][x] = SP_CELL_FOOD;
                food_count++;
                break;
            case 'D':
                game->cells[y][x] = SP_CELL_DOOR;
                game->door_x = x;
                game->door_y = y;
                door_count++;
                break;
            case 'E':
                game->cells[y][x] = SP_CELL_EXIT;
                game->exit_x = x;
                game->exit_y = y;
                exit_count++;
                break;
            case '@':
                game->cells[y][x] = SP_CELL_EMPTY;
                start_x = x;
                start_y = y;
                head_count++;
                break;
            default:
                snprintf(error, error_size, "%s: unknown cell '%c' at %d,%d", path, c, x, y);
                return false;
            }
        }
    }
    if (head_count != 1) {
        snprintf(error, error_size, "%s: map needs exactly one '@'", path);
        return false;
    }
    if (door_count != 1 && exit_count != 1) {
        snprintf(error, error_size, "%s: map needs one door or one exit", path);
        return false;
    }
    if (food_count < 1) {
        snprintf(error, error_size, "%s: map needs at least one food", path);
        return false;
    }
    if (!place_initial_snake(game, start_x, start_y)) {
        snprintf(error, error_size, "%s: no room for the initial snake body", path);
        return false;
    }
    game->turn_food_total = food_count;
    game->seed = 0;
    {
        int saved_exit_x = game->exit_x;
        int saved_exit_y = game->exit_y;
        reset_run_fields(game);
        if (exit_count == 1) {
            game->exit_x = saved_exit_x;
            game->exit_y = saved_exit_y;
        }
    }
    game->mode = exit_count == 1 && door_count == 0 ? SP_MODE_REALTIME : SP_MODE_TURN;
    set_quota(game);
    snprintf(game->message, sizeof(game->message), "Loaded %s.", path);
    return true;
}

bool game_door_open(const SpGame *game)
{
    return game->mode == SP_MODE_TURN &&
           game->turn_food_eaten >= game->turn_food_min &&
           game->turn_food_eaten <= game->turn_food_max;
}

static bool generate_realtime(SpGame *game, uint64_t seed)
{
    int attempt;
    int carry_length = clamp_int(game->length, 3, 12);

    for (attempt = 0; attempt < 96; ++attempt) {
        uint64_t rng = seed + (uint64_t)attempt * UINT64_C(0xD1B54A32D192ED03);
        int head_x = 13;
        int head_y = 7;
        int exit_x;
        int exit_y;
        int food_count = 5;
        int i;

        fill_base_map(game, 20, 15);
        exit_x = game->width - 3;
        exit_y = 3;
        game->cells[exit_y][exit_x] = SP_CELL_EXIT;
        clear_zone(game, head_x, head_y, 2);
        clear_zone(game, exit_x, exit_y, 2);
        for (i = 1; i <= head_x + 2 && i < game->width - 1; ++i) {
            game->cells[head_y][i] = SP_CELL_EMPTY;
        }
        for (i = 0; i < carry_length; ++i) {
            game->cells[head_y][head_x - i] = SP_CELL_EMPTY;
        }
        add_random_walls(game, &rng, (game->width * game->height) / 9, 1, 5, 15, 9);
        for (i = 1; i <= head_x + 2 && i < game->width - 1; ++i) {
            game->cells[head_y][i] = SP_CELL_EMPTY;
        }
        clear_zone(game, exit_x, exit_y, 2);
        game->cells[exit_y][exit_x] = SP_CELL_EXIT;
        game->door_x = -1;
        game->door_y = -1;
        game->exit_x = exit_x;
        game->exit_y = exit_y;
        if (!place_snake_dir(game, head_x, head_y, SP_DIR_RIGHT, carry_length)) {
            continue;
        }
        if (!reachable_contains(game, exit_x, exit_y)) {
            continue;
        }
        if (place_food(game, &rng, food_count) != food_count) {
            continue;
        }
        game->mode = SP_MODE_REALTIME;
        game->dir = SP_DIR_RIGHT;
        game->realtime_ticks = 0;
        game->end_ms = 0;
        if (!autoplay_realtime_solves(game)) {
            continue;
        }
        set_message(game, "The world shifts. Reach the exit.");
        return true;
    }
    return false;
}

bool game_enter_realtime(SpGame *game)
{
    uint64_t seed;
    SpGame candidate;

    if (game->mode != SP_MODE_TURN) {
        return false;
    }
    seed = game->seed != 0 ? game->seed : (uint64_t)game->turn_food_eaten * 131u + 7u;
    candidate = *game;
    if (!generate_realtime(&candidate, seed ^ UINT64_C(0xC0FFEE))) {
        set_message(game, "The real-time chamber failed to generate.");
        return false;
    }
    candidate.phase_bonus += 100;
    candidate.mode = SP_MODE_REALTIME;
    set_message(&candidate, "Door crossed: REAL-TIME MODE. Reach the exit.");
    *game = candidate;
    return true;
}

static void finish_dead(SpGame *game, const char *reason)
{
    game->mode = SP_MODE_DEAD;
    set_message(game, reason);
}

SpStep game_step(SpGame *game, SpDir dir)
{
    int nx;
    int ny;
    unsigned char cell;
    bool food;
    bool blocked;
    int i;
    int tail;

    if (game->mode != SP_MODE_TURN && game->mode != SP_MODE_REALTIME) {
        return SP_STEP_BLOCKED;
    }
    if (dir == SP_DIR_NONE) {
        return SP_STEP_BLOCKED;
    }
    if (dir == sp_dir_opposite(game->dir)) {
        set_message(game, "Reverse is not allowed.");
        return SP_STEP_BLOCKED;
    }
    nx = game->snake[0].x + DX[dir];
    ny = game->snake[0].y + DY[dir];
    if (!in_bounds(game, nx, ny)) {
        if (game->mode == SP_MODE_REALTIME) {
            finish_dead(game, "CRASH: outside the arena.");
            return SP_STEP_DEAD;
        }
        set_message(game, "Blocked: outside the map.");
        return SP_STEP_BLOCKED;
    }
    cell = game->cells[ny][nx];
    food = cell == SP_CELL_FOOD;
    blocked = false;

    if (cell == SP_CELL_WALL) {
        blocked = true;
    } else if (cell == SP_CELL_DOOR) {
        if (game->mode == SP_MODE_TURN && !game_door_open(game)) {
            if (game->turn_food_eaten < game->turn_food_min) {
                snprintf(game->message, sizeof(game->message),
                         "Door locked: eat %d-%d food (you have %d).", game->turn_food_min,
                         game->turn_food_max, game->turn_food_eaten);
            } else {
                snprintf(game->message, sizeof(game->message),
                         "Door sealed: you ate too much (%d > %d).", game->turn_food_eaten,
                         game->turn_food_max);
            }
            return SP_STEP_BLOCKED;
        }
        if (game->mode == SP_MODE_REALTIME) {
            blocked = true;
        }
    }

    tail = game->length - 1;
    for (i = 0; i < game->length; ++i) {
        if (game->snake[i].x == nx && game->snake[i].y == ny) {
            if (!(i == tail && !food)) {
                blocked = true;
            }
            break;
        }
    }

    if (blocked) {
        if (game->mode == SP_MODE_REALTIME) {
            finish_dead(game, cell == SP_CELL_WALL ? "CRASH: wall." : "CRASH: your body.");
            return SP_STEP_DEAD;
        }
        set_message(game, "Blocked by a wall, the door, or your body.");
        return SP_STEP_BLOCKED;
    }

    if (food) {
        if (game->length < SP_SNAKE_MAX) {
            game->length += 1;
        }
        for (i = game->length - 1; i > 0; --i) {
            game->snake[i] = game->snake[i - 1];
        }
        game->snake[0].x = nx;
        game->snake[0].y = ny;
        game->cells[ny][nx] = SP_CELL_EMPTY;
        game->turn_food_eaten += 1;
        game->total_food_eaten += 1;
        if (game->length > game->max_length) {
            game->max_length = game->length;
        }
        game->moves += 1;
        game->dir = dir;
        snprintf(game->message, sizeof(game->message), "Food! Length %d.", game->length);
    } else {
        for (i = game->length - 1; i > 0; --i) {
            game->snake[i] = game->snake[i - 1];
        }
        game->snake[0].x = nx;
        game->snake[0].y = ny;
        game->moves += 1;
        game->dir = dir;
        if (cell == SP_CELL_DOOR) {
            set_message(game, "Door entered.");
            return SP_STEP_DOOR;
        }
        if (cell == SP_CELL_EXIT) {
            game->mode = SP_MODE_WON;
            game->phase_bonus += 250;
            set_message(game, "EXIT REACHED. Run complete.");
            return SP_STEP_EXIT;
        }
        set_message(game, "");
    }

    if (game->mode == SP_MODE_REALTIME) {
        game->realtime_ticks += 1;
    }
    return SP_STEP_OK;
}

int game_elapsed_seconds(const SpGame *game, uint64_t now_ms)
{
    uint64_t end = game->end_ms != 0 ? game->end_ms : now_ms;
    if (game->start_ms == 0 || end <= game->start_ms) {
        return 0;
    }
    return (int)((end - game->start_ms) / 1000u);
}

int game_time_bonus(const SpGame *game, uint64_t now_ms)
{
    int seconds = game_elapsed_seconds(game, now_ms);
    int remaining = 300 - seconds;
    return remaining > 0 ? remaining * 2 : 0;
}

int game_score(const SpGame *game, uint64_t now_ms)
{
    return game->total_food_eaten * 10 + game->max_length * 20 + game->phase_bonus +
           game_time_bonus(game, now_ms);
}

bool game_parse_commands(const char *text, SpCommand *out, int max_commands, int *out_count,
                         char *error, size_t error_size)
{
    const char *p = text;
    int count = 0;

    if (out_count != NULL) {
        *out_count = 0;
    }
    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }
    if (text == NULL) {
        snprintf(error, error_size, "empty command");
        return false;
    }
    while (*p != '\0') {
        SpDir dir;
        int steps = 1;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        dir = sp_dir_from_char(*p);
        if (dir == SP_DIR_NONE) {
            snprintf(error, error_size, "expected w/a/s/d, found '%c'", *p);
            return false;
        }
        ++p;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p >= '0' && *p <= '9') {
            steps = 0;
            while (*p >= '0' && *p <= '9') {
                steps = steps * 10 + (*p - '0');
                if (steps > SP_MAX_STEPS) {
                    snprintf(error, error_size, "steps must be 1-%d", SP_MAX_STEPS);
                    return false;
                }
                ++p;
            }
        }
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p != '\0' && *p != ';') {
            snprintf(error, error_size, "unexpected '%c' after command", *p);
            return false;
        }
        if (steps < 1) {
            snprintf(error, error_size, "steps must be 1-%d", SP_MAX_STEPS);
            return false;
        }
        if (count >= max_commands) {
            snprintf(error, error_size, "too many commands (max %d)", max_commands);
            return false;
        }
        out[count].dir = dir;
        out[count].steps = steps;
        ++count;
        if (*p == ';') {
            ++p;
            if (*p == '\0') {
                break;
            }
        }
    }
    if (count == 0) {
        snprintf(error, error_size, "no commands supplied");
        return false;
    }
    if (out_count != NULL) {
        *out_count = count;
    }
    return true;
}

static bool bfs_cell_blocked(const SpGame *game, int x, int y, unsigned char target)
{
    unsigned char cell = game->cells[y][x];

    if (cell == SP_CELL_WALL) {
        return true;
    }
    if (body_blocks(game, x, y)) {
        return true;
    }
    if (cell == SP_CELL_DOOR) {
        return target != SP_CELL_DOOR || !game_door_open(game);
    }
    if (cell == SP_CELL_EXIT) {
        return target != SP_CELL_EXIT;
    }
    if (cell == SP_CELL_FOOD && target == SP_CELL_DOOR) {
        return true;
    }
    return false;
}

SpDir bfs_next_direction(const SpGame *game)
{
    SpPos queue[SP_MAP_W * SP_MAP_H];
    unsigned char seen[SP_MAP_H][SP_MAP_W];
    SpDir first[SP_MAP_H][SP_MAP_W];
    unsigned char target;
    int head = 0;
    int tail = 0;
    int d;

    if (game->mode == SP_MODE_REALTIME) {
        target = SP_CELL_EXIT;
    } else if (game->mode == SP_MODE_TURN) {
        if (game->turn_food_eaten < game->turn_food_min) {
            target = SP_CELL_FOOD;
        } else if (game->turn_food_eaten <= game->turn_food_max) {
            target = SP_CELL_DOOR;
        } else {
            return SP_DIR_NONE;
        }
    } else {
        return SP_DIR_NONE;
    }
    memset(seen, 0, sizeof(seen));
    memset(first, 0, sizeof(first));
    if (game->length < 1) {
        return SP_DIR_NONE;
    }
    queue[tail++] = game->snake[0];
    seen[game->snake[0].y][game->snake[0].x] = 1;
    while (head < tail) {
        SpPos cur = queue[head++];
        for (d = 0; d < 4; ++d) {
            int nx = cur.x + DX[d];
            int ny = cur.y + DY[d];
            if (!in_bounds(game, nx, ny) || seen[ny][nx]) {
                continue;
            }
            if (bfs_cell_blocked(game, nx, ny, target)) {
                continue;
            }
            seen[ny][nx] = 1;
            first[ny][nx] = (cur.x == game->snake[0].x && cur.y == game->snake[0].y)
                                ? (SpDir)d
                                : first[cur.y][cur.x];
            if (game->cells[ny][nx] == target) {
                return first[ny][nx];
            }
            queue[tail].x = nx;
            queue[tail].y = ny;
            tail++;
        }
    }
    return SP_DIR_NONE;
}

SpDir player_move(const SpGame *game)
{
    /*
     * This is the strategy entry point. Edit this function and rebuild to
     * submit a different autoplay policy. The default policy is BFS toward
     * the next required food, then the door / real-time exit.
     */
    return bfs_next_direction(game);
}
