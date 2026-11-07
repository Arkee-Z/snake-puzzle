#include "snake_puzzle/core/board.h"

#include <string.h>

void board_init(Board *board, int width, int height)
{
    int x;
    int y;

    width = sp_clamp_int(width, 3, SP_MAX_WIDTH);
    height = sp_clamp_int(height, 3, SP_MAX_HEIGHT);
    memset(board, 0, sizeof(*board));
    board->width = width;
    board->height = height;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            board->cells[y][x].type = TERRAIN_FLOOR;
            board->cells[y][x].gate = -1;
            board->cells[y][x].plate = -1;
            board->cells[y][x].teleport = -1;
            board->cells[y][x].seal = -1;
            board->cells[y][x].orb = -1;
            board->cells[y][x].oneway_dir = DIR_NONE;
        }
    }
}

void board_fill(Board *board, Terrain terrain)
{
    int x;
    int y;

    for (y = 0; y < board->height; ++y) {
        for (x = 0; x < board->width; ++x) {
            board->cells[y][x].type = terrain;
            board->cells[y][x].gate = -1;
            board->cells[y][x].plate = -1;
            board->cells[y][x].teleport = -1;
            board->cells[y][x].seal = -1;
            board->cells[y][x].orb = -1;
            board->cells[y][x].oneway_dir = DIR_NONE;
            board->cells[y][x].break_triggered = false;
        }
    }
}

bool board_in_bounds(const Board *board, int x, int y)
{
    return x >= 0 && y >= 0 && x < board->width && y < board->height;
}

Cell *board_cell(Board *board, int x, int y)
{
    if (!board_in_bounds(board, x, y)) {
        return NULL;
    }
    return &board->cells[y][x];
}

const Cell *board_cell_const(const Board *board, int x, int y)
{
    if (!board_in_bounds(board, x, y)) {
        return NULL;
    }
    return &board->cells[y][x];
}

bool board_floor(Board *board, int x, int y)
{
    Cell *cell = board_cell(board, x, y);

    return cell != NULL && cell->type == TERRAIN_FLOOR;
}
