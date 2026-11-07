#include "snake_puzzle/gen/solver.h"

#include <stdio.h>
#include <string.h>

typedef struct SearchNode {
    int x;
    int y;
    int dir;
} SearchNode;

static bool cell_blocked_for_search(const Game *game, int x, int y, Direction move_dir,
                                    bool ignore_mini_exit)
{
    const Cell *cell = board_cell_const(&game->board, x, y);

    if (cell == NULL) {
        return true;
    }
    if (cell->type == TERRAIN_WALL || cell->type == TERRAIN_PIT) {
        return true;
    }
    if (game_block_at((Game *)game, x, y) != NULL) {
        return true;
    }
    if (cell->type == TERRAIN_ONEWAY && (Direction)cell->oneway_dir != move_dir) {
        return true;
    }
    if (cell->type == TERRAIN_LOCKED) {
        if (cell->gate < 0 || cell->gate >= game->gate_count ||
            !game->gates[cell->gate].open) {
            return true;
        }
    }
    if (cell->type == TERRAIN_SPECIAL_GATE) {
        if (cell->gate < 0 || cell->gate >= game->gate_count ||
            !game->gates[cell->gate].open || game->gates[cell->gate].used) {
            return true;
        }
    }
    if (cell->type == TERRAIN_MINI_EXIT && !ignore_mini_exit) {
        if (cell->gate < 0 || cell->gate >= game->gate_count ||
            !game->gates[cell->gate].open) {
            return true;
        }
    }
    return false;
}

bool solver_reachable(const Game *game, Point from, Point to, bool ignore_mini_exit,
                      int *out_distance)
{
    static int distance[SP_MAX_HEIGHT][SP_MAX_WIDTH][4];
    static SearchNode queue[SP_MAX_HEIGHT * SP_MAX_WIDTH * 4];
    int head = 0;
    int tail = 0;
    int start_dir;
    int d;

    if (!board_in_bounds(&game->board, from.x, from.y) ||
        !board_in_bounds(&game->board, to.x, to.y)) {
        return false;
    }
    if (from.x == to.x && from.y == to.y) {
        if (out_distance != NULL) {
            *out_distance = 0;
        }
        return true;
    }
    memset(distance, -1, sizeof(distance));
    start_dir = (int)game->snake.dir;
    if (start_dir < 0 || start_dir > 3) {
        start_dir = DIR_UP;
    }
    distance[from.y][from.x][start_dir] = 0;
    queue[tail++] = (SearchNode){from.x, from.y, start_dir};
    while (head < tail) {
        SearchNode node = queue[head++];
        for (d = 0; d < 4; ++d) {
            Direction dir = (Direction)d;
            int nx = node.x + dir_dx(dir);
            int ny = node.y + dir_dy(dir);
            int dist = distance[node.y][node.x][node.dir];

            if (!board_in_bounds(&game->board, nx, ny)) {
                continue;
            }
            if (cell_blocked_for_search(game, nx, ny, dir, ignore_mini_exit)) {
                continue;
            }
            if (distance[ny][nx][dir] < 0) {
                distance[ny][nx][dir] = dist + 1;
                if (nx == to.x && ny == to.y) {
                    if (out_distance != NULL) {
                        *out_distance = dist + 1;
                    }
                    return true;
                }
                queue[tail++] = (SearchNode){nx, ny, d};
            }
        }
    }
    return false;
}

bool solver_all_targets_reachable(const Game *game, char *error, size_t error_size)
{
    Point head = game->snake.body[0];
    int i;

    for (i = 0; i < game->orb_count; ++i) {
        if (!solver_reachable(game, head, (Point){game->orbs[i].x, game->orbs[i].y}, true,
                              NULL)) {
            snprintf(error, error_size, "orb %d at %d,%d cannot be reached", i + 1,
                     game->orbs[i].x, game->orbs[i].y);
            return false;
        }
    }
    for (i = 0; i < game->gate_count; ++i) {
        if (game->gates[i].kind == GATE_MINI_EXIT) {
            if (!solver_reachable(game, head, (Point){game->gates[i].x, game->gates[i].y},
                                  true, NULL)) {
                snprintf(error, error_size, "chamber exit at %d,%d cannot be reached",
                         game->gates[i].x, game->gates[i].y);
                return false;
            }
        }
    }
    return true;
}

bool solver_verify_reference(Game *game, const CommandSequence *reference, SolverResult *result)
{
    Game copy;
    int command_index;
    int step_index;
    int total_steps = 0;

    memset(result, 0, sizeof(*result));
    if (game == NULL || reference == NULL || reference->count == 0) {
        snprintf(result->error, sizeof(result->error), "missing reference solution");
        return false;
    }
    copy = *game;
    if (copy.phase != GAME_PLAYING || copy.snake.length <= 0) {
        snprintf(result->error, sizeof(result->error), "level is not in a playable state");
        return false;
    }
    for (command_index = 0; command_index < reference->count; ++command_index) {
        Command command = reference->commands[command_index];

        for (step_index = 0; step_index < command.steps; ++step_index) {
            StepResult step = game_step(&copy, command.dir);
            total_steps += 1;
            if (step == STEP_DIED) {
                snprintf(result->error, sizeof(result->error),
                         "reference solution died after %d steps: %s", total_steps,
                         copy.death_reason);
                return false;
            }
            if (step == STEP_SPECIAL_ENTERED) {
                result->ok = true;
                result->reached_special = true;
                result->steps_executed = total_steps;
                result->commands_executed = command_index + 1;
                return true;
            }
            if (step == STEP_MINI_COMPLETE) {
                result->ok = true;
                result->steps_executed = total_steps;
                result->commands_executed = command_index + 1;
                return true;
            }
            if (step == STEP_INVALID) {
                snprintf(result->error, sizeof(result->error),
                         "reference solution reached an invalid state at step %d", total_steps);
                return false;
            }
        }
    }
    result->steps_executed = total_steps;
    result->commands_executed = reference->count;
    snprintf(result->error, sizeof(result->error),
             "reference solution never reached the special gate");
    return false;
}
