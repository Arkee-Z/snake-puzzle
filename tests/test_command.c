#include <string.h>

#include "snake_puzzle/core/command.h"
#include "test.h"

int test_command_run(void)
{
    CommandSequence sequence;
    char error[256];
    char formatted[256];

    SP_CHECK(command_parse("w 3; d 2; s 1; a 4", &sequence, error, sizeof(error)));
    SP_CHECK_INT(sequence.count, 4);
    SP_CHECK_INT(sequence.commands[0].dir, DIR_UP);
    SP_CHECK_INT(sequence.commands[0].steps, 3);
    SP_CHECK_INT(sequence.commands[1].dir, DIR_RIGHT);
    SP_CHECK_INT(sequence.commands[3].dir, DIR_LEFT);
    SP_CHECK_INT(command_sequence_total_steps(&sequence), 10);
    command_sequence_format(&sequence, formatted, sizeof(formatted));
    SP_CHECK(strcmp(formatted, "w 3; d 2; s 1; a 4") == 0);

    SP_CHECK(command_parse("  D 2 ; W 9 ; ", &sequence, error, sizeof(error)));
    SP_CHECK_INT(sequence.count, 2);
    SP_CHECK_INT(sequence.commands[0].dir, DIR_RIGHT);
    SP_CHECK_INT(sequence.commands[1].dir, DIR_UP);
    SP_CHECK_INT(sequence.commands[1].steps, 9);

    SP_CHECK(!command_parse("", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("x 3", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w 0", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w 10", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w3", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w 3 d 2", &sequence, error, sizeof(error)));
    SP_CHECK(!command_parse("w 3;; d 2", &sequence, error, sizeof(error)));
    SP_CHECK(error[0] != '\0');

    command_sequence_clear(&sequence);
    SP_CHECK(command_sequence_push(&sequence, DIR_DOWN, 23));
    SP_CHECK_INT(sequence.count, 3);
    SP_CHECK_INT(sequence.commands[0].steps, 9);
    SP_CHECK_INT(sequence.commands[1].steps, 9);
    SP_CHECK_INT(sequence.commands[2].steps, 5);
    SP_CHECK_INT(command_sequence_total_steps(&sequence), 23);
    return 0;
}
