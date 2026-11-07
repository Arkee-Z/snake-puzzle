#include "snake_puzzle/gen/generator.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_puzzle/gen/dependency.h"
#include "snake_puzzle/gen/validator.h"

#ifndef SP_DEFAULT_ATOMS_DIR
#define SP_DEFAULT_ATOMS_DIR "assets/atoms"
#endif

static void set_error(char *error, size_t error_size, const char *format, ...)
{
    va_list args;

    if (error == NULL || error_size == 0) {
        return;
    }
    va_start(args, format);
    vsnprintf(error, error_size, format, args);
    va_end(args);
}

bool generator_init(Generator *generator, const char *atoms_path, char *error,
                    size_t error_size)
{
    const char *path = atoms_path;

    memset(generator, 0, sizeof(*generator));
    if (path == NULL || *path == '\0') {
        path = getenv("SNAKE_PUZZLE_ATOMS");
    }
    if (path == NULL || *path == '\0') {
        path = SP_DEFAULT_ATOMS_DIR;
    }
    if (!atom_load_path(&generator->library, path, error, error_size)) {
        return false;
    }
    return true;
}

static int choose_atom(const AtomLibrary *library, Rng *rng, const int *excluded,
                       int excluded_count)
{
    int candidates[SP_ATOM_MAX];
    int weights[SP_ATOM_MAX];
    int count = 0;
    int total = 0;
    int i;
    int roll;

    for (i = 0; i < library->count; ++i) {
        bool skip = false;
        int j;
        for (j = 0; j < excluded_count; ++j) {
            if (i == excluded[j]) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            candidates[count] = i;
            weights[count] = library->atoms[i].weight;
            total += weights[count];
            count += 1;
        }
    }
    if (count == 0) {
        return -1;
    }
    roll = rng_int(rng, 1, total);
    for (i = 0; i < count; ++i) {
        roll -= weights[i];
        if (roll <= 0) {
            return candidates[i];
        }
    }
    return candidates[count - 1];
}

static int choose_atom_pair(const AtomLibrary *library, Rng *rng, int max_total_width,
                            int *first, int *second)
{
    int excluded[1];
    int attempts;
    int a = -1;
    int b = -1;

    for (attempts = 0; attempts < 32; ++attempts) {
        a = choose_atom(library, rng, NULL, 0);
        if (a < 0) {
            return 0;
        }
        excluded[0] = a;
        b = choose_atom(library, rng, excluded, 1);
        if (b < 0) {
            b = a;
        }
        if (library->atoms[a].width + library->atoms[b].width <= max_total_width &&
            library->atoms[a].entry_y == library->atoms[b].entry_y &&
            library->atoms[a].height == library->atoms[b].height) {
            *first = a;
            *second = b;
            return 2;
        }
    }
    return 0;
}

static void append_atom_solution(CommandSequence *witness, const Atom *atom)
{
    int i;

    for (i = 0; i < atom->solution.count; ++i) {
        command_sequence_push(witness, atom->solution.commands[i].dir,
                              atom->solution.commands[i].steps);
    }
}

static void carve_cell(Game *game, int x, int y)
{
    Cell *cell = board_cell(&game->board, x, y);
    if (cell != NULL && cell->type == TERRAIN_WALL) {
        game_set_cell(game, x, y, TERRAIN_FLOOR);
    }
}

static int carve_branch(Game *game, Rng *rng, int left, int right, int top,
                        int branch_index, int *food_x, int *food_y, int *reward_x,
                        int *reward_y)
{
    int min_x = left + 1;
    int max_x = right - 1;
    int bx;
    int dx;
    int x;
    int y;
    int mid;
    int cells = 0;

    if (top < 1 || top > game->board.height - 2 || max_x - min_x < 1) {
        return 0;
    }
    bx = rng_int(rng, min_x, max_x - 1);
    dx = rng_int(rng, bx + 1, max_x);
    if (top < 7) {
        for (y = top; y <= 6; ++y) {
            carve_cell(game, bx, y);
            carve_cell(game, dx, y);
            cells += 2;
        }
        for (x = bx; x <= dx; ++x) {
            carve_cell(game, x, top);
            cells += 1;
        }
    } else {
        for (y = 8; y <= top; ++y) {
            carve_cell(game, bx, y);
            carve_cell(game, dx, y);
            cells += 2;
        }
        for (x = bx; x <= dx; ++x) {
            carve_cell(game, x, top);
            cells += 1;
        }
    }
    mid = (bx + dx) / 2;
    *reward_x = mid;
    *reward_y = top;
    *food_x = bx;
    *food_y = top < 7 ? top + 1 : top - 1;
    SP_UNUSED(branch_index);
    return cells;
}

static void add_branch_reward(Game *game, Rng *rng, int x, int y, bool golden)
{
    if (golden) {
        game_add_food(game, x, y, FOOD_GOLDEN);
    } else {
        ItemType type = (ItemType)rng_int(rng, 0, ITEM_COUNT - 1);
        game_add_item(game, x, y, type);
    }
}

static bool try_generate(Generator *generator, const GenOptions *options, Game *out_game,
                         GenStats *stats, char *error, size_t error_size)
{
    int width = options->width > 0 ? options->width : SP_DEFAULT_WIDTH;
    int height = options->height > 0 ? options->height : SP_DEFAULT_HEIGHT;
    uint64_t attempt_seed = rng_mix64(options->seed ^ UINT64_C(0xBADC0FFEE0DDF00D));
    Rng rng;
    int first;
    int second;
    int origin_y;
    int atom2_x;
    int last_exit_x;
    int gate_x;
    int branch_left;
    int branch_right;
    int branch_count;
    int branch_cells = 0;
    int food_x1 = -1;
    int food_y1 = -1;
    int reward_x1 = -1;
    int reward_y1 = -1;
    int food_x2 = -1;
    int food_y2 = -1;
    int reward_x2 = -1;
    int reward_y2 = -1;
    const Atom *atom1;
    const Atom *atom2;
    char local_error[SP_MESSAGE_MAX];
    CommandSequence witness;
    ValidationResult validation;
    int floor_count = 0;
    int total_cells;
    int x;
    int y;

    if (width < 16 || width > SP_MAX_WIDTH || height < 11 || height > SP_MAX_HEIGHT) {
        set_error(error, error_size,
                  "generator needs a board between 16x11 and %dx%d, got %dx%d",
                  SP_MAX_WIDTH, SP_MAX_HEIGHT, width, height);
        return false;
    }
    if (generator->library.count == 0) {
        set_error(error, error_size, "atom library is empty");
        return false;
    }
    rng_seed(&rng, attempt_seed);
    if (choose_atom_pair(&generator->library, &rng, 12, &first, &second) != 2) {
        set_error(error, error_size, "no two puzzle atoms fit the chosen board width");
        return false;
    }
    atom1 = &generator->library.atoms[first];
    atom2 = &generator->library.atoms[second];
    origin_y = 7 - atom1->entry_y;
    atom2_x = 2 + atom1->width;
    if (origin_y < 0 || origin_y + atom1->height > height - 1 ||
        origin_y + atom2->height > height - 1) {
        set_error(error, error_size, "selected atoms do not fit vertically");
        return false;
    }
    if (atom2_x + atom2->width > width - 2) {
        set_error(error, error_size, "selected atoms do not fit horizontally");
        return false;
    }

    game_init(out_game, width, height, options->seed, GAME_MAIN);
    board_fill(&out_game->board, TERRAIN_WALL);

    for (y = 6; y <= 10; ++y) {
        carve_cell(out_game, 1, y);
    }
    if (!atom_stamp(atom1, 2, origin_y, out_game, local_error, sizeof(local_error))) {
        set_error(error, error_size, "%s", local_error);
        return false;
    }
    if (!atom_stamp(atom2, atom2_x, origin_y, out_game, local_error, sizeof(local_error))) {
        set_error(error, error_size, "%s", local_error);
        return false;
    }
    last_exit_x = atom2_x + atom2->exit_x;
    if (game_add_seal(out_game, 2 + atom1->exit_x, origin_y + atom1->exit_y) < 0 ||
        game_add_seal(out_game, last_exit_x, origin_y + atom2->exit_y) < 0) {
        set_error(error, error_size, "could not place seals");
        return false;
    }

    gate_x = width - 2;
    for (x = last_exit_x + 1; x < gate_x; ++x) {
        carve_cell(out_game, x, 7);
    }
    if (game_add_gate(out_game, GATE_SPECIAL, gate_x, 7, 2, 0) < 0) {
        set_error(error, error_size, "could not place the special gate");
        return false;
    }

    branch_left = last_exit_x + 1;
    branch_right = gate_x - 1;
    branch_count = 0;
    if (branch_right - branch_left >= 2) {
        branch_count = 1;
        branch_cells += carve_branch(out_game, &rng, branch_left, branch_right, 3, 0,
                                     &food_x1, &food_y1, &reward_x1, &reward_y1);
    }
    if (height >= 13 && branch_right - branch_left >= 4 && rng_range(&rng, 100) < 55) {
        if (carve_branch(out_game, &rng, branch_left, branch_right, 11, 1, &food_x2,
                         &food_y2, &reward_x2, &reward_y2) > 0) {
            branch_count += 1;
        }
    }
    if (reward_x1 >= 0) {
        add_branch_reward(out_game, &rng, reward_x1, reward_y1, false);
    }
    if (food_x1 >= 0) {
        game_add_food(out_game, food_x1, food_y1, FOOD_NORMAL);
    }
    if (reward_x2 >= 0) {
        add_branch_reward(out_game, &rng, reward_x2, reward_y2, true);
    }
    if (food_x2 >= 0) {
        game_add_food(out_game, food_x2, food_y2, FOOD_NORMAL);
    }
    if (food_x1 < 0 && food_x2 < 0) {
        if (out_game->board.cells[6][1].type != TERRAIN_WALL) {
            game_add_food(out_game, 1, 6, FOOD_NORMAL);
        } else {
            game_add_food(out_game, 1, 10, FOOD_NORMAL);
        }
    }

    if (!game_place_snake(out_game, (Point){1, 7}, DIR_UP, 3)) {
        set_error(error, error_size, "start area is blocked");
        return false;
    }

    command_sequence_clear(&witness);
    command_sequence_push(&witness, DIR_RIGHT, 1);
    append_atom_solution(&witness, atom1);
    command_sequence_push(&witness, DIR_RIGHT, 1);
    append_atom_solution(&witness, atom2);
    command_sequence_push(&witness, DIR_RIGHT, gate_x - last_exit_x);
    if (witness.count > SP_MAX_COMMANDS) {
        set_error(error, error_size, "reference solution exceeds command capacity");
        return false;
    }

    if (!validator_validate_level(out_game, &witness, &validation)) {
        set_error(error, error_size, "%s [%s + %s]", validation.error, atom1->name,
                  atom2->name);
        return false;
    }
    if (!validation.witness_solved) {
        set_error(error, error_size, "reference solution did not enter the special gate");
        return false;
    }

    memset(stats, 0, sizeof(*stats));
    stats->valid = true;
    stats->seed = options->seed;
    stats->attempts = 1;
    {
        DependencyGraph graph;
        dependency_build_linear(&graph, 2);
        if (!dependency_is_acyclic(&graph)) {
            set_error(error, error_size, "internal dependency graph is not acyclic");
            return false;
        }
        stats->chain_length = dependency_chain_length(&graph);
    }
    stats->core_atoms = 2;
    stats->seal_count = 2;
    stats->branch_count = branch_count;
    stats->branch_cells = branch_cells;
    stats->witness = witness;
    stats->witness_commands = witness.count;
    stats->witness_steps = command_sequence_total_steps(&witness);
    stats->placements[0].x = 2;
    stats->placements[0].y = origin_y;
    stats->placements[0].width = atom1->width;
    stats->placements[0].height = atom1->height;
    snprintf(stats->placements[0].name, sizeof(stats->placements[0].name), "%s", atom1->name);
    stats->placements[1].x = atom2_x;
    stats->placements[1].y = origin_y;
    stats->placements[1].width = atom2->width;
    stats->placements[1].height = atom2->height;
    snprintf(stats->placements[1].name, sizeof(stats->placements[1].name), "%s", atom2->name);
    snprintf(stats->atoms[0], sizeof(stats->atoms[0]), "%s", atom1->name);
    snprintf(stats->atoms[1], sizeof(stats->atoms[1]), "%s", atom2->name);
    total_cells = width * height;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            if (out_game->board.cells[y][x].type != TERRAIN_WALL) {
                floor_count += 1;
            }
        }
    }
    stats->floor_density = total_cells > 0 ? (double)floor_count / (double)total_cells : 0.0;
    return true;
}

bool generator_generate(Generator *generator, const GenOptions *options, Game *game,
                        GenStats *stats, char *error, size_t error_size)
{
    int attempt;
    char last_error[SP_MESSAGE_MAX] = "generator failed";
    GenOptions local = *options;

    if (local.width <= 0) {
        local.width = SP_DEFAULT_WIDTH;
    }
    if (local.height <= 0) {
        local.height = SP_DEFAULT_HEIGHT;
    }
    for (attempt = 0; attempt < SP_GEN_MAX_ATTEMPTS; ++attempt) {
        GenStats trial;
        Game candidate;

        local.seed = options->seed + (uint64_t)attempt * UINT64_C(0x9E3779B97F4A7C15);
        if (try_generate(generator, &local, &candidate, &trial, last_error,
                         sizeof(last_error))) {
            trial.seed = options->seed;
            trial.attempts = attempt + 1;
            candidate.seed = options->seed;
            *game = candidate;
            *stats = trial;
            return true;
        }
        if (getenv("SP_DEBUG_GEN") != NULL) {
            fprintf(stderr, "generator attempt %d (seed %llu): %s\n", attempt + 1,
                    (unsigned long long)options->seed, last_error);
        }
    }
    if (stats != NULL) {
        memset(stats, 0, sizeof(*stats));
        stats->valid = false;
        stats->seed = options->seed;
        stats->attempts = SP_GEN_MAX_ATTEMPTS;
    }
    set_error(error, error_size, "generation failed after %d attempts: %s",
              SP_GEN_MAX_ATTEMPTS, last_error);
    return false;
}

static int open_neighbors(const Game *game, int x, int y)
{
    static const int DX[4] = {0, 1, 0, -1};
    static const int DY[4] = {-1, 0, 1, 0};
    int count = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        const Cell *cell = board_cell_const(&game->board, x + DX[i], y + DY[i]);
        if (cell != NULL && cell->type != TERRAIN_WALL && cell->type != TERRAIN_PIT) {
            count += 1;
        }
    }
    return count;
}

bool generator_generate_special(uint64_t seed, int width, int height, Game *game, char *error,
                                size_t error_size)
{
    int attempt;

    if (width <= 0) {
        width = 13;
    }
    if (height <= 0) {
        height = 9;
    }
    width = sp_clamp_int(width, 11, SP_MAX_WIDTH);
    height = sp_clamp_int(height, 9, SP_MAX_HEIGHT);
    for (attempt = 0; attempt < 32; ++attempt) {
        Rng rng;
        int wall_count;
        int i;
        int x;
        int y;
        int body_y;
        int orbs_to_place = 3;
        int placed = 0;

        game_init(game, width, height, rng_mix64(seed + (uint64_t)attempt), GAME_SPECIAL);
        board_fill(&game->board, TERRAIN_WALL);
        for (y = 1; y < height - 1; ++y) {
            for (x = 1; x < width - 1; ++x) {
                game_set_cell(game, x, y, TERRAIN_FLOOR);
            }
        }
        rng_seed(&rng, rng_mix64(seed ^ UINT64_C(0xC0FFEE) ^ (uint64_t)attempt));
        wall_count = rng_int(&rng, 5, 11);
        for (i = 0; i < wall_count; ++i) {
            x = rng_int(&rng, 4, width - 3);
            y = rng_int(&rng, 1, height - 2);
            if (y >= 4 && y <= 7 && x <= 4) {
                continue;
            }
            if (x >= width - 4 && y <= 2) {
                continue;
            }
            game_set_cell(game, x, y, TERRAIN_WALL);
        }
        for (body_y = 4; body_y <= 7; ++body_y) {
            for (x = 1; x <= 3; ++x) {
                game_set_cell(game, x, body_y, TERRAIN_FLOOR);
            }
        }
        if (!game_place_snake(game, (Point){2, 5}, DIR_UP, 3)) {
            continue;
        }
        game_set_cell(game, width - 3, 2, TERRAIN_FLOOR);
        game_set_cell(game, width - 4, 2, TERRAIN_FLOOR);
        game_set_cell(game, width - 3, 1, TERRAIN_FLOOR);
        game_set_cell(game, width - 3, 3, TERRAIN_FLOOR);
        if (game_add_gate(game, GATE_MINI_EXIT, width - 3, 2, 0, orbs_to_place) < 0) {
            continue;
        }
        for (i = 0; i < 300 && placed < orbs_to_place; ++i) {
            x = rng_int(&rng, 2, width - 2);
            y = rng_int(&rng, 2, height - 2);
            if ((x == width - 3 && y == 2) || (x <= 4 && y >= 4 && y <= 7)) {
                continue;
            }
            {
                const Cell *cell = board_cell_const(&game->board, x, y);
                bool occupied = false;
                int j;
                if (cell == NULL || cell->type != TERRAIN_FLOOR) {
                    continue;
                }
                if (open_neighbors(game, x, y) < 2) {
                    continue;
                }
                for (j = 0; j < game->orb_count; ++j) {
                    if (game->orbs[j].x == x && game->orbs[j].y == y) {
                        occupied = true;
                    }
                }
                if (!occupied && game_add_orb(game, x, y) >= 0) {
                    placed += 1;
                }
            }
        }
        if (placed != orbs_to_place) {
            continue;
        }
        if (validator_validate_special(game, error, error_size)) {
            game->seed = seed;
            return true;
        }
    }
    set_error(error, error_size, "could not generate a solvable special chamber");
    return false;
}
