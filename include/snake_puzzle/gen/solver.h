#ifndef SNAKE_PUZZLE_GEN_SOLVER_H
#define SNAKE_PUZZLE_GEN_SOLVER_H

#include "snake_puzzle/core/command.h"
#include "snake_puzzle/core/game.h"

typedef struct SolverResult {
    bool ok;
    bool reached_special;
    int steps_executed;
    int commands_executed;
    char error[SP_MESSAGE_MAX];
} SolverResult;

bool solver_verify_reference(Game *game, const CommandSequence *reference, SolverResult *result);
bool solver_reachable(const Game *game, Point from, Point to, bool ignore_mini_exit,
                      int *out_distance);
bool solver_all_targets_reachable(const Game *game, char *error, size_t error_size);

#endif
