#ifndef SNAKE_PUZZLE_CORE_RNG_H
#define SNAKE_PUZZLE_CORE_RNG_H

#include <stdint.h>

typedef struct Rng {
    uint64_t state;
} Rng;

void rng_seed(Rng *rng, uint64_t seed);
uint64_t rng_next(Rng *rng);
uint32_t rng_range(Rng *rng, uint32_t bound);
int rng_int(Rng *rng, int min_inclusive, int max_inclusive);
uint64_t rng_seed_from_string(const char *text);
uint64_t rng_mix64(uint64_t value);

#endif
