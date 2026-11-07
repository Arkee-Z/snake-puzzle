#ifndef SNAKE_PUZZLE_GEN_GENERATOR_H
#define SNAKE_PUZZLE_GEN_GENERATOR_H

#include "snake_puzzle/gen/atom.h"
#include "snake_puzzle/gen/dependency.h"

#define SP_GEN_MAX_ATOMS 3
#define SP_GEN_MAX_ATTEMPTS 48

typedef struct PlacedAtomInfo {
    char name[SP_ATOM_NAME];
    int x;
    int y;
    int width;
    int height;
} PlacedAtomInfo;

typedef struct GenOptions {
    int width;
    int height;
    uint64_t seed;
    const char *atoms_path;
} GenOptions;

typedef struct GenStats {
    bool valid;
    uint64_t seed;
    int attempts;
    int core_atoms;
    int chain_length;
    int seal_count;
    int branch_count;
    int witness_steps;
    int witness_commands;
    int branch_cells;
    double floor_density;
    char atoms[SP_GEN_MAX_ATOMS][SP_ATOM_NAME];
    PlacedAtomInfo placements[SP_GEN_MAX_ATOMS];
    CommandSequence witness;
} GenStats;

typedef struct Generator {
    AtomLibrary library;
} Generator;

bool generator_init(Generator *generator, const char *atoms_path, char *error,
                    size_t error_size);
bool generator_generate(Generator *generator, const GenOptions *options, Game *game,
                        GenStats *stats, char *error, size_t error_size);
bool generator_generate_special(uint64_t seed, int width, int height, Game *game, char *error,
                                size_t error_size);

#endif
