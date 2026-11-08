#include <stdio.h>
#include <string.h>

#include "sp.h"

static int checks = 0;
static int failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        ++checks;                                                                              \
        if (!(condition)) {                                                                    \
            ++failures;                                                                        \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);              \
        }                                                                                      \
    } while (0)

#define CHECK_INT(actual, expected)                                                            \
    do {                                                                                       \
        int a_ = (int)(actual);                                                                \
        int e_ = (int)(expected);                                                              \
        ++checks;                                                                              \
        if (a_ != e_) {                                                                        \
            ++failures;                                                                        \
            fprintf(stderr, "FAIL %s:%d: %s == %d, expected %d\n", __FILE__, __LINE__,        \
                    #actual, a_, e_);                                                          \
        }                                                                                      \
    } while (0)

static void blank_game(SpGame *game, int width, int height)
{
    int x;
    int y;

    game_new(game);
    game->width = width;
    game->height = height;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            bool border = x == 0 || y == 0 || x == width - 1 || y == height - 1;
            game->cells[y][x] = border ? SP_CELL_WALL : SP_CELL_EMPTY;
        }
    }
    game->mode = SP_MODE_TURN;
    game->turn_food_total = 5;
    game->turn_food_min = 2;
    game->turn_food_max = 4;
}

static void set_straight_snake(SpGame *game, int x, int y, SpDir dir, int length)
{
    static const int dx[4] = {0, 1, 0, -1};
    static const int dy[4] = {-1, 0, 1, 0};
    int i;

    game->length = length;
    game->dir = dir;
    for (i = 0; i < length; ++i) {
        game->snake[i].x = x - dx[dir] * i;
        game->snake[i].y = y - dy[dir] * i;
    }
    if (game->max_length < length) {
        game->max_length = length;
    }
}

static void test_parser(void)
{
    SpCommand commands[SP_MAX_COMMANDS];
    int count = 0;
    char error[128];

    CHECK(game_parse_commands("w", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
    CHECK_INT(count, 1);
    CHECK_INT(commands[0].dir, SP_DIR_UP);
    CHECK_INT(commands[0].steps, 1);

    CHECK(game_parse_commands("w 3; d 2; a 1", commands, SP_MAX_COMMANDS, &count, error,
                              sizeof(error)));
    CHECK_INT(count, 3);
    CHECK_INT(commands[0].steps, 3);
    CHECK_INT(commands[1].dir, SP_DIR_RIGHT);
    CHECK_INT(commands[2].dir, SP_DIR_LEFT);

    CHECK(game_parse_commands("d12", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
    CHECK_INT(commands[0].steps, 12);

    CHECK(!game_parse_commands("", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
    CHECK(!game_parse_commands("x 2", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
    CHECK(!game_parse_commands("w 0", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
    CHECK(!game_parse_commands("w 100", commands, SP_MAX_COMMANDS, &count, error, sizeof(error)));
}

static void test_tutorial_map(void)
{
    SpGame game;
    char error[128];

    CHECK(game_load_map(&game, "maps/tutorial.map", error, sizeof(error)));
    if (game.width != 20) {
        fprintf(stderr, "load error: %s\n", error);
    }
    CHECK_INT(game.width, 20);
    CHECK_INT(game.height, 15);
    CHECK_INT(game.turn_food_total, 5);
    CHECK_INT(game.turn_food_min, 2);
    CHECK_INT(game.turn_food_max, 4);
    CHECK_INT(game.door_x, 18);
    CHECK_INT(game.door_y, 13);
    CHECK_INT(game.length, 3);
    CHECK_INT(game.dir, SP_DIR_UP);
    CHECK_INT(game.snake[0].x, 2);
    CHECK_INT(game.snake[0].y, 2);
}

static void test_movement_and_food(void)
{
    SpGame game;

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 3, 3, SP_DIR_RIGHT, 3);
    game.cells[3][4] = SP_CELL_FOOD;
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_OK);
    CHECK_INT(game.snake[0].x, 4);
    CHECK_INT(game.length, 4);
    CHECK_INT(game.total_food_eaten, 1);
    CHECK_INT(game.turn_food_eaten, 1);

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 3, 1, SP_DIR_UP, 3);
    CHECK_INT(game_step(&game, SP_DIR_UP), SP_STEP_BLOCKED);
    CHECK_INT(game.snake[0].y, 1);
    CHECK_INT(game.mode, SP_MODE_TURN);

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 3, 3, SP_DIR_RIGHT, 3);
    game.cells[3][5] = SP_CELL_DOOR;
    game.door_x = 5;
    game.door_y = 3;
    game.turn_food_eaten = 2;
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_OK);
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_DOOR);
    CHECK_INT(game.mode, SP_MODE_TURN);
}

static void test_door_quota(void)
{
    SpGame game;

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 3, 3, SP_DIR_RIGHT, 3);
    game.cells[3][4] = SP_CELL_DOOR;
    game.door_x = 4;
    game.door_y = 3;
    game.turn_food_eaten = 1;
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_BLOCKED);
    CHECK_INT(game.mode, SP_MODE_TURN);

    game.turn_food_eaten = 2;
    CHECK(game_door_open(&game));
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_DOOR);
}

static void test_self_collision(void)
{
    SpGame game;

    blank_game(&game, 8, 8);
    game.length = 7;
    game.dir = SP_DIR_UP;
    game.snake[0] = (SpPos){3, 3};
    game.snake[1] = (SpPos){3, 4};
    game.snake[2] = (SpPos){4, 4};
    game.snake[3] = (SpPos){4, 3};
    game.snake[4] = (SpPos){4, 2};
    game.snake[5] = (SpPos){3, 2};
    game.snake[6] = (SpPos){2, 2};
    CHECK_INT(game_step(&game, SP_DIR_UP), SP_STEP_BLOCKED);
    CHECK_INT(game.snake[0].x, 3);
    CHECK_INT(game.snake[0].y, 3);
}

static void test_bfs(void)
{
    SpGame game;
    SpPos destination;

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 1, 1, SP_DIR_RIGHT, 3);
    game.cells[1][4] = SP_CELL_FOOD;
    game.turn_food_min = 1;
    CHECK_INT(bfs_next_direction(&game), SP_DIR_RIGHT);

    game.turn_food_eaten = 2;
    game.cells[1][6] = SP_CELL_DOOR;
    game.door_x = 6;
    game.door_y = 1;
    CHECK_INT(bfs_next_direction(&game), SP_DIR_RIGHT);
    (void)destination;
}

static void test_enter_realtime(void)
{
    SpGame game;
    int old_length;

    blank_game(&game, 8, 8);
    set_straight_snake(&game, 3, 3, SP_DIR_RIGHT, 5);
    game.seed = 42;
    old_length = game.length;
    game.phase_bonus = 0;
    CHECK(game_enter_realtime(&game));
    CHECK_INT(game.mode, SP_MODE_REALTIME);
    CHECK_INT(game.length, old_length);
    CHECK_INT(game.phase_bonus, 100);
    CHECK_INT(game.width, 20);
    CHECK_INT(game.height, 15);
    CHECK(game.exit_x > 0 && game.exit_y > 0);
    CHECK(game.cells[game.exit_y][game.exit_x] == SP_CELL_EXIT);
}

static void test_realtime_death_and_exit(void)
{
    SpGame game;

    blank_game(&game, 8, 8);
    game.mode = SP_MODE_REALTIME;
    set_straight_snake(&game, 3, 1, SP_DIR_UP, 3);
    CHECK_INT(game_step(&game, SP_DIR_UP), SP_STEP_DEAD);
    CHECK_INT(game.mode, SP_MODE_DEAD);

    blank_game(&game, 8, 8);
    game.mode = SP_MODE_REALTIME;
    set_straight_snake(&game, 3, 3, SP_DIR_RIGHT, 3);
    game.cells[3][4] = SP_CELL_EXIT;
    game.exit_x = 4;
    game.exit_y = 3;
    CHECK_INT(game_step(&game, SP_DIR_RIGHT), SP_STEP_EXIT);
    CHECK_INT(game.mode, SP_MODE_WON);
    CHECK_INT(game.phase_bonus, 250);
}

static void test_score_and_determinism(void)
{
    SpGame game;
    SpGame again;
    int expected;

    blank_game(&game, 8, 8);
    game.start_ms = 1000;
    game.end_ms = 1000 + 10000;
    game.total_food_eaten = 3;
    game.max_length = 6;
    game.phase_bonus = 100;
    expected = 30 + 120 + 100 + (300 - 10) * 2;
    CHECK_INT(game_score(&game, 0), expected);
    CHECK_INT(game_elapsed_seconds(&game, 0), 10);

    CHECK(game_generate_turn(&game, 20, 15, 12345));
    CHECK(game_generate_turn(&again, 20, 15, 12345));
    CHECK_INT(game.width, again.width);
    CHECK_INT(game.height, again.height);
    CHECK_INT(game.turn_food_total, again.turn_food_total);
    CHECK(memcmp(game.cells, again.cells, sizeof(game.cells)) == 0);
    CHECK(memcmp(game.snake, again.snake, sizeof(game.snake)) == 0);
    CHECK_INT(game.length, again.length);
    CHECK_INT(game.snake[0].x, again.snake[0].x);
    CHECK_INT(game.snake[0].y, again.snake[0].y);
}

static void test_generated_autoplay(void)
{
    int seed;

    for (seed = 0; seed < 20; ++seed) {
        SpGame game;
        int steps;
        bool door = false;
        bool exit_found = false;

        CHECK(game_generate_turn(&game, 20, 15, (uint64_t)seed));
        game_start(&game, 1);
        for (steps = 0; steps < 500; ++steps) {
            SpDir dir = player_move(&game);
            SpStep step;
            if (dir == SP_DIR_NONE) {
                break;
            }
            step = game_step(&game, dir);
            if (step == SP_STEP_DOOR) {
                door = true;
                break;
            }
            if (step != SP_STEP_OK) {
                break;
            }
        }
        CHECK(door);
        CHECK(game_enter_realtime(&game));
        for (steps = 0; steps < 500; ++steps) {
            SpDir dir = player_move(&game);
            SpStep step;
            if (dir == SP_DIR_NONE) {
                break;
            }
            step = game_step(&game, dir);
            if (step == SP_STEP_EXIT) {
                exit_found = true;
                break;
            }
            if (step != SP_STEP_OK) {
                break;
            }
        }
        CHECK(exit_found);
    }
}

int main(void)
{
    test_parser();
    test_tutorial_map();
    test_movement_and_food();
    test_door_quota();
    test_self_collision();
    test_bfs();
    test_enter_realtime();
    test_realtime_death_and_exit();
    test_score_and_determinism();
    test_generated_autoplay();

    printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
