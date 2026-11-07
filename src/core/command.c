#include "snake_puzzle/core/command.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void command_sequence_clear(CommandSequence *sequence)
{
    memset(sequence, 0, sizeof(*sequence));
}

static bool is_space_char(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool command_parse(const char *line, CommandSequence *out, char *error, size_t error_size)
{
    const char *p = line;
    int index = 0;

    command_sequence_clear(out);
    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }
    if (line == NULL) {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "missing command input");
        }
        return false;
    }

    while (is_space_char(*p)) {
        ++p;
    }
    if (*p == '\0') {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "no commands supplied");
        }
        return false;
    }

    while (*p != '\0') {
        Direction dir;
        int steps;

        if (out->count >= SP_MAX_COMMANDS) {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "too many commands (max %d)", SP_MAX_COMMANDS);
            }
            return false;
        }

        dir = dir_from_char(*p);
        if (dir == DIR_NONE) {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size,
                         "command %d: expected w/a/s/d, found '%c'", index + 1, *p);
            }
            return false;
        }
        ++p;
        if (!is_space_char(*p)) {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size,
                         "command %d: put a space between direction and steps", index + 1);
            }
            return false;
        }
        while (is_space_char(*p)) {
            ++p;
        }
        if (*p < '1' || *p > '9') {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "command %d: steps must be a single digit 1-9",
                         index + 1);
            }
            return false;
        }
        steps = *p - '0';
        ++p;
        if (isdigit((unsigned char)*p)) {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "command %d: steps must be 1-9", index + 1);
            }
            return false;
        }

        out->commands[out->count].dir = dir;
        out->commands[out->count].steps = steps;
        out->count += 1;
        ++index;

        while (is_space_char(*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        if (*p != ';') {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size,
                         "command %d: expected ';' between commands, found '%c'", index, *p);
            }
            return false;
        }
        ++p;
        while (is_space_char(*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
    }

    if (out->count == 0) {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "no commands supplied");
        }
        return false;
    }
    return true;
}

bool command_sequence_push(CommandSequence *sequence, Direction dir, int steps)
{
    while (steps > 0) {
        int chunk = steps > 9 ? 9 : steps;

        if (sequence->count >= SP_MAX_COMMANDS) {
            return false;
        }
        sequence->commands[sequence->count].dir = dir;
        sequence->commands[sequence->count].steps = chunk;
        sequence->count += 1;
        steps -= chunk;
    }
    return true;
}

int command_sequence_total_steps(const CommandSequence *sequence)
{
    int i;
    int total = 0;

    for (i = 0; i < sequence->count; ++i) {
        total += sequence->commands[i].steps;
    }
    return total;
}

void command_sequence_format(const CommandSequence *sequence, char *out, size_t out_size)
{
    size_t used = 0;
    int i;

    if (out == NULL || out_size == 0) {
        return;
    }
    out[0] = '\0';
    for (i = 0; i < sequence->count; ++i) {
        int written = snprintf(out + used, out_size - used, "%s%c %d",
                               i == 0 ? "" : "; ", dir_to_char(sequence->commands[i].dir),
                               sequence->commands[i].steps);
        if (written < 0 || (size_t)written >= out_size - used) {
            out[out_size - 1] = '\0';
            return;
        }
        used += (size_t)written;
    }
}
