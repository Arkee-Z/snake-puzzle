#ifndef SNAKE_PUZZLE_CORE_BOARD_H
#define SNAKE_PUZZLE_CORE_BOARD_H

#include "snake_puzzle/entity/terrain.h"

typedef struct Cell {
    Terrain type;
    int8_t gate;            /* index into Game.gates, -1 when unused */
    int8_t plate;           /* index into Game.plates */
    int8_t teleport;        /* index into Game.teleports */
    int8_t seal;            /* index into Game.seals */
    int8_t orb;             /* index into Game.orbs */
    uint8_t oneway_dir;     /* Direction */
    bool break_triggered;
} Cell;

typedef struct Board {
    int width;
    int height;
    Cell cells[SP_MAX_HEIGHT][SP_MAX_WIDTH];
} Board;

void board_init(Board *board, int width, int height);
void board_fill(Board *board, Terrain terrain);
bool board_in_bounds(const Board *board, int x, int y);
Cell *board_cell(Board *board, int x, int y);
const Cell *board_cell_const(const Board *board, int x, int y);
bool board_floor(Board *board, int x, int y);

#endif
