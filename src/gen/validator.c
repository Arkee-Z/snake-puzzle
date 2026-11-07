#include "snake_puzzle/gen/validator.h"

#include <stdio.h>
#include <string.h>

#include "snake_puzzle/gen/solver.h"

bool validator_validate_level(const Game *game, const CommandSequence *witness,
                              ValidationResult *result)
{
    SolverResult solve_result;
    int i;
    int special_count = 0;

    memset(result, 0, sizeof(*result));
    if (game == NULL || witness == NULL) {
        snprintf(result->error, sizeof(result->error), "missing level or reference solution");
        return false;
    }
    if (game->kind != GAME_MAIN) {
        snprintf(result->error, sizeof(result->error), "main-level validator received wrong kind");
        return false;
    }
    if (game->phase != GAME_PLAYING || game->snake.length < 1) {
        snprintf(result->error, sizeof(result->error), "snake is not alive at level start");
        return false;
    }
    for (i = 0; i < game->snake.length; ++i) {
        const Cell *cell =
            board_cell_const(&game->board, game->snake.body[i].x, game->snake.body[i].y);
        if (cell == NULL || cell->type == TERRAIN_WALL) {
            snprintf(result->error, sizeof(result->error),
                     "snake body starts inside a wall at %d,%d", game->snake.body[i].x,
                     game->snake.body[i].y);
            return false;
        }
    }
    for (i = 0; i < game->gate_count; ++i) {
        if (game->gates[i].kind == GATE_SPECIAL) {
            special_count += 1;
            if (game->gates[i].required_seals > game->seal_count) {
                snprintf(result->error, sizeof(result->error),
                         "special gate requires more seals than exist");
                return false;
            }
        }
    }
    if (special_count < 1) {
        snprintf(result->error, sizeof(result->error), "no special gate in level");
        return false;
    }
    if (game->seal_count < 1) {
        snprintf(result->error, sizeof(result->error), "no seals placed");
        return false;
    }
    if (!solver_verify_reference((Game *)game, witness, &solve_result)) {
        snprintf(result->error, sizeof(result->error), "%s", solve_result.error);
        return false;
    }
    if (!solve_result.reached_special) {
        snprintf(result->error, sizeof(result->error),
                 "reference solution finished without entering the special gate");
        return false;
    }
    result->ok = true;
    result->witness_solved = true;
    result->witness_steps = solve_result.steps_executed;
    result->seal_count = game->seal_count;
    result->gate_count = game->gate_count;
    return true;
}

bool validator_validate_special(const Game *game, char *error, size_t error_size)
{
    int i;
    int exits = 0;

    if (game == NULL || game->kind != GAME_SPECIAL) {
        snprintf(error, error_size, "wrong game kind for special chamber");
        return false;
    }
    if (game->snake.length < 1 || game->phase != GAME_PLAYING) {
        snprintf(error, error_size, "special chamber snake is not alive");
        return false;
    }
    if (game->orb_count < 1) {
        snprintf(error, error_size, "special chamber has no orbs");
        return false;
    }
    for (i = 0; i < game->orb_count; ++i) {
        const Cell *cell = board_cell_const(&game->board, game->orbs[i].x, game->orbs[i].y);
        int dx[4] = {0, 1, 0, -1};
        int dy[4] = {-1, 0, 1, 0};
        int open_count = 0;
        int d;
        if (cell == NULL || cell->type != TERRAIN_ORB) {
            snprintf(error, error_size, "orb %d is not on the board", i + 1);
            return false;
        }
        for (d = 0; d < 4; ++d) {
            const Cell *neighbor = board_cell_const(&game->board, game->orbs[i].x + dx[d],
                                                    game->orbs[i].y + dy[d]);
            if (neighbor != NULL && neighbor->type != TERRAIN_WALL) {
                open_count += 1;
            }
        }
        if (open_count < 2) {
            snprintf(error, error_size, "orb %d sits in a dead-end cell", i + 1);
            return false;
        }
    }
    for (i = 0; i < game->gate_count; ++i) {
        if (game->gates[i].kind == GATE_MINI_EXIT) {
            exits += 1;
        }
    }
    if (exits != 1) {
        snprintf(error, error_size, "special chamber needs exactly one exit");
        return false;
    }
    if (!solver_all_targets_reachable(game, error, error_size)) {
        return false;
    }
    return true;
}
