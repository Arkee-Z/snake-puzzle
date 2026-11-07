#ifndef SNAKE_PUZZLE_GEN_VALIDATOR_H
#define SNAKE_PUZZLE_GEN_VALIDATOR_H

#include "snake_puzzle/core/command.h"
#include "snake_puzzle/core/game.h"

typedef struct ValidationResult {
    bool ok;
    bool witness_solved;
    int witness_steps;
    int seal_count;
    int gate_count;
    char error[SP_MESSAGE_MAX];
} ValidationResult;

bool validator_validate_level(const Game *game, const CommandSequence *witness,
                              ValidationResult *result);
bool validator_validate_special(const Game *game, char *error, size_t error_size);

#endif
