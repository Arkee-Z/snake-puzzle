#include <string.h>

#include "snake_puzzle/core/game.h"
#include "snake_puzzle/core/score.h"
#include "test.h"

static void flat_game(Game *game, int width, int height, Point head, Direction dir, int length)
{
    int x;
    int y;

    game_init(game, width, height, 99, GAME_MAIN);
    board_fill(&game->board, TERRAIN_FLOOR);
    for (x = 0; x < width; ++x) {
        game_set_cell(game, x, 0, TERRAIN_WALL);
        game_set_cell(game, x, height - 1, TERRAIN_WALL);
    }
    for (y = 0; y < height; ++y) {
        game_set_cell(game, 0, y, TERRAIN_WALL);
        game_set_cell(game, width - 1, y, TERRAIN_WALL);
    }
    SP_CHECK(game_place_snake(game, head, dir, length));
}

static void set_body(Game *game, Point head, Direction dir, const Point *body, int length)
{
    int i;

    game->snake.length = length;
    game->snake.dir = dir;
    game->snake.alive = true;
    game->snake.body[0] = head;
    for (i = 1; i < length; ++i) {
        game->snake.body[i] = body[i - 1];
    }
}

int test_game_run(void)
{
    Game game;
    StepResult result;

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_UP, 3);
    game_add_food(&game, 3, 2, FOOD_NORMAL);
    result = game_step(&game, DIR_UP);
    SP_CHECK_INT(result, STEP_OK);
    SP_CHECK_INT(game.snake.length, 4);
    SP_CHECK_INT(game.score, SP_SCORE_FOOD_NORMAL);
    SP_CHECK_INT(game.foods_eaten, 1);
    SP_CHECK(game_food_at(&game, 3, 2) == NULL);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_UP, 3);
    game_set_cell(&game, 3, 2, TERRAIN_WALL);
    result = game_step(&game, DIR_UP);
    SP_CHECK_INT(result, STEP_DIED);
    SP_CHECK_INT(game.phase, GAME_DEAD);
    SP_CHECK(strstr(game.death_reason, "wall") != NULL);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_UP, 3);
    result = game_step(&game, DIR_DOWN);
    SP_CHECK_INT(result, STEP_DIED);
    SP_CHECK(strstr(game.death_reason, "reversed") != NULL);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_UP, 3);
    game_set_cell(&game, 3, 2, TERRAIN_WALL);
    game.shields = 1;
    result = game_step(&game, DIR_UP);
    SP_CHECK_INT(result, STEP_OK);
    SP_CHECK_INT(game.phase, GAME_PLAYING);
    SP_CHECK_INT(game.shields, 0);
    SP_CHECK_INT(game.snake.body[0].y, 3);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    game_add_item(&game, 4, 3, ITEM_KEY);
    {
        int gate = game_add_gate(&game, GATE_LOCKED, 5, 3, 0, 0);
        SP_CHECK(gate >= 0);
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
        SP_CHECK_INT(game.keys, 1);
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
        SP_CHECK_INT(game.keys, 0);
        SP_CHECK(game.gates[gate].open);
        SP_CHECK_INT(game.snake.body[0].x, 5);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    {
        Cell *cell = board_cell(&game.board, 4, 3);
        cell->type = TERRAIN_ONEWAY;
        cell->oneway_dir = DIR_LEFT;
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_DIED);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    {
        Cell *cell = board_cell(&game.board, 4, 3);
        cell->type = TERRAIN_ONEWAY;
        cell->oneway_dir = DIR_RIGHT;
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
        SP_CHECK_INT(game.snake.body[0].x, 4);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    {
        int gate = game_add_gate(&game, GATE_LOCKED, 7, 3, 0, 0);
        int plate = game_add_plate(&game, 5, 3, UINT64_C(1) << gate);
        game_add_block(&game, 4, 3);
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
        SP_CHECK_INT(game.blocks[0].x, 5);
        SP_CHECK(game.plates[plate].pressed);
        SP_CHECK(game.gates[gate].open);
        SP_CHECK_INT(game.snake.body[0].x, 4);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    game_set_cell(&game, 4, 3, TERRAIN_BREAK);
    game_set_cell(&game, 5, 3, TERRAIN_BREAK);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(board_cell(&game.board, 4, 3)->type, TERRAIN_PIT);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    game_add_teleport(&game, 4, 3, 6, 5);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game.snake.body[0].x, 6);
    SP_CHECK_INT(game.snake.body[0].y, 5);

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    game_set_cell(&game, 4, 3, TERRAIN_WALL);
    game.phase_tokens = 1;
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game.phase_tokens, 0);
    SP_CHECK_INT(game.snake.body[0].x, 4);
    SP_CHECK_INT(board_cell(&game.board, 4, 3)->type, TERRAIN_FLOOR);

    flat_game(&game, 9, 9, (Point){5, 5}, DIR_UP, 1);
    {
        Point body[] = {{5, 6}, {4, 6}, {4, 5}, {5, 4}, {6, 4}};
        set_body(&game, (Point){5, 5}, DIR_UP, body, 6);
        SP_CHECK_INT(game_step(&game, DIR_UP), STEP_DIED);
        SP_CHECK(strstr(game.death_reason, "own body") != NULL);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    {
        int gate;
        int seal = game_add_seal(&game, 4, 3);
        SP_CHECK(seal >= 0);
        gate = game_add_gate(&game, GATE_SPECIAL, 5, 3, 1, 0);
        SP_CHECK(gate >= 0);
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
        SP_CHECK_INT(game.seals_collected, 1);
        SP_CHECK(game.gates[gate].open);
        SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_SPECIAL_ENTERED);
        SP_CHECK_INT(game.pending_special_gate, gate);
        SP_CHECK(game_finish_special(&game, 77));
        SP_CHECK_INT(game.pending_special_gate, -1);
        SP_CHECK(game.gates[gate].used);
        SP_CHECK_INT(game.score, 77 + SP_SCORE_SEAL_BASE);
        SP_CHECK_INT(board_cell(&game.board, 5, 3)->type, TERRAIN_FLOOR);
    }

    flat_game(&game, 9, 9, (Point){3, 3}, DIR_RIGHT, 3);
    game_add_item(&game, 4, 3, ITEM_DOUBLE);
    game_add_food(&game, 5, 3, FOOD_GOLDEN);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK(game.double_next);
    SP_CHECK_INT(game_step(&game, DIR_RIGHT), STEP_OK);
    SP_CHECK_INT(game.score, SP_SCORE_ITEM + SP_SCORE_FOOD_GOLDEN * 2);
    SP_CHECK(!game.double_next);

    return 0;
}
