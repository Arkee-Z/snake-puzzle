#ifndef SNAKE_PUZZLE_PLATFORM_H
#define SNAKE_PUZZLE_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

bool sp_executable_path(char *out, size_t out_size);
bool sp_executable_dir(char *out, size_t out_size);
bool sp_resolve_atoms_path(const char *requested, bool requested_explicit, char *out,
                           size_t out_size);

#endif
