#ifndef SNAKE_PUZZLE_GEN_ATOM_H
#define SNAKE_PUZZLE_GEN_ATOM_H

#include "snake_puzzle/core/command.h"
#include "snake_puzzle/core/game.h"
#include "snake_puzzle/core/rng.h"

typedef struct Atom {
    char name[SP_ATOM_NAME];
    int width;
    int height;
    int difficulty;
    int weight;
    int entry_x;
    int entry_y;
    int exit_x;
    int exit_y;
    char cells[SP_ATOM_MAX_H][SP_ATOM_MAX_W + 1];
    CommandSequence solution;
    char source_path[SP_ATOM_PATH];
} Atom;

typedef struct AtomLibrary {
    Atom atoms[SP_ATOM_MAX];
    int count;
    char source[SP_ATOM_PATH];
} AtomLibrary;

bool atom_load_path(AtomLibrary *library, const char *path, char *error, size_t error_size);
bool atom_load_file(AtomLibrary *library, const char *path, char *error, size_t error_size);
bool atom_stamp(const Atom *atom, int origin_x, int origin_y, Game *game, char *error,
                size_t error_size);
const Atom *atom_find(const AtomLibrary *library, const char *name);
void atom_library_summary(const AtomLibrary *library, char *out, size_t out_size);

#endif
