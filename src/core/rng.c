#include "snake_puzzle/core/rng.h"

#include <string.h>

void rng_seed(Rng *rng, uint64_t seed)
{
    rng->state = seed;
}

uint64_t rng_next(Rng *rng)
{
    uint64_t z;

    rng->state += UINT64_C(0x9E3779B97F4A7C15);
    z = rng->state;
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

uint32_t rng_range(Rng *rng, uint32_t bound)
{
    uint64_t limit;
    uint64_t value;

    if (bound <= 1U) {
        return 0U;
    }
    limit = UINT64_MAX - (UINT64_MAX % (uint64_t)bound);
    do {
        value = rng_next(rng);
    } while (value >= limit);
    return (uint32_t)(value % (uint64_t)bound);
}

int rng_int(Rng *rng, int min_inclusive, int max_inclusive)
{
    if (max_inclusive <= min_inclusive) {
        return min_inclusive;
    }
    return min_inclusive + (int)rng_range(rng, (uint32_t)(max_inclusive - min_inclusive + 1));
}

uint64_t rng_seed_from_string(const char *text)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    const unsigned char *p = (const unsigned char *)text;

    while (p != NULL && *p != '\0') {
        hash ^= (uint64_t)(*p++);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

uint64_t rng_mix64(uint64_t value)
{
    value += UINT64_C(0x9E3779B97F4A7C15);
    value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
    return value ^ (value >> 31);
}
