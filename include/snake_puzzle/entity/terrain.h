#ifndef SNAKE_PUZZLE_ENTITY_TERRAIN_H
#define SNAKE_PUZZLE_ENTITY_TERRAIN_H

#include "snake_puzzle/common.h"

typedef enum Terrain {
    TERRAIN_FLOOR = 0,
    TERRAIN_WALL,
    TERRAIN_ONEWAY,
    TERRAIN_LOCKED,
    TERRAIN_SPECIAL_GATE,
    TERRAIN_BREAK,
    TERRAIN_PIT,
    TERRAIN_PLATE,
    TERRAIN_SEAL,
    TERRAIN_TELEPORT,
    TERRAIN_MINI_EXIT,
    TERRAIN_ORB,
    TERRAIN_COUNT
} Terrain;

typedef struct TerrainDef {
    const char *name;
    char glyph;
    bool passable;
    bool block_passable;
    bool phaseable;
} TerrainDef;

typedef struct Plate {
    int x;
    int y;
    bool pressed;
    uint64_t gate_mask;
} Plate;

typedef struct TeleportPair {
    int x1;
    int y1;
    int x2;
    int y2;
    bool active;
} TeleportPair;

typedef struct Block {
    int x;
    int y;
    bool active;
} Block;

const TerrainDef *terrain_def(Terrain terrain);
const char *terrain_name(Terrain terrain);

#endif
