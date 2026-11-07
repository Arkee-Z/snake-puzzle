#include "snake_puzzle/core/board.h"
#include "test.h"

int test_board_run(void)
{
    Board board;
    Cell *cell;

    board_init(&board, 8, 6);
    SP_CHECK_INT(board.width, 8);
    SP_CHECK_INT(board.height, 6);
    SP_CHECK(board_in_bounds(&board, 0, 0));
    SP_CHECK(board_in_bounds(&board, 7, 5));
    SP_CHECK(!board_in_bounds(&board, 8, 5));
    SP_CHECK(!board_in_bounds(&board, -1, 0));
    SP_CHECK(terrain_def(TERRAIN_FLOOR)->passable);
    SP_CHECK(!terrain_def(TERRAIN_WALL)->passable);
    SP_CHECK(terrain_def(TERRAIN_WALL)->phaseable);

    cell = board_cell(&board, 3, 2);
    SP_CHECK(cell != NULL);
    cell->type = TERRAIN_WALL;
    SP_CHECK(!board_floor(&board, 3, 2));
    board_fill(&board, TERRAIN_WALL);
    SP_CHECK(!board_floor(&board, 0, 0));
    SP_CHECK(board_cell(&board, -1, 0) == NULL);
    return 0;
}
