#ifndef SNAKE_PUZZLE_CORE_COMMAND_H
#define SNAKE_PUZZLE_CORE_COMMAND_H

#include "snake_puzzle/common.h"

typedef struct Command {
    Direction dir;
    int steps;
} Command;

typedef struct CommandSequence {
    Command commands[SP_MAX_COMMANDS];
    int count;
} CommandSequence;

void command_sequence_clear(CommandSequence *sequence);
bool command_parse(const char *line, CommandSequence *out, char *error, size_t error_size);
bool command_sequence_push(CommandSequence *sequence, Direction dir, int steps);
int command_sequence_total_steps(const CommandSequence *sequence);
void command_sequence_format(const CommandSequence *sequence, char *out, size_t out_size);

#endif
