#include "snake_puzzle/entity/terrain.h"

static const TerrainDef TERRAIN_DEFS[TERRAIN_COUNT] = {
    [TERRAIN_FLOOR] = {"floor", '.', true, true, false},
    [TERRAIN_WALL] = {"wall", '#', false, false, true},
    [TERRAIN_ONEWAY] = {"one-way gate", '>', true, false, false},
    [TERRAIN_LOCKED] = {"locked door", 'L', false, false, false},
    [TERRAIN_SPECIAL_GATE] = {"special gate", 'D', false, false, false},
    [TERRAIN_BREAK] = {"break floor", '~', true, false, false},
    [TERRAIN_PIT] = {"pit", 'x', false, false, false},
    [TERRAIN_PLATE] = {"pressure plate", '_', true, true, false},
    [TERRAIN_SEAL] = {"seal", 'S', true, false, false},
    [TERRAIN_TELEPORT] = {"teleporter", 'T', true, false, false},
    [TERRAIN_MINI_EXIT] = {"chamber exit", 'E', false, false, false},
    [TERRAIN_ORB] = {"orb", 'O', true, false, false},
};

const TerrainDef *terrain_def(Terrain terrain)
{
    if (terrain < 0 || terrain >= TERRAIN_COUNT) {
        return &TERRAIN_DEFS[TERRAIN_WALL];
    }
    return &TERRAIN_DEFS[terrain];
}

const char *terrain_name(Terrain terrain)
{
    return terrain_def(terrain)->name;
}
