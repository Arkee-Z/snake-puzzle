#include <string.h>

#include "snake_puzzle/app.h"
#include "test.h"

static void flat_special(Game *game)
{
    int x;
    int y;

    game_init(game, 13, 9, 7, GAME_SPECIAL);
    board_fill(&game->board, TERRAIN_FLOOR);
    for (x = 0; x < game->board.width; ++x) {
        game_set_cell(game, x, 0, TERRAIN_WALL);
        game_set_cell(game, x, game->board.height - 1, TERRAIN_WALL);
    }
    for (y = 0; y < game->board.height; ++y) {
        game_set_cell(game, 0, y, TERRAIN_WALL);
        game_set_cell(game, game->board.width - 1, y, TERRAIN_WALL);
    }
    SP_CHECK(game_place_snake(game, (Point){3, 5}, DIR_RIGHT, 3));
    game_add_orb(game, 4, 5);
    game_add_orb(game, 5, 5);
    game_add_orb(game, 6, 5);
    game_add_gate(game, GATE_MINI_EXIT, 7, 5, 0, 3);
}

int test_app_run(void)
{
    App app;
    AppOptions options;
    char error[SP_MESSAGE_MAX];
    char commands[SP_LINE_MAX];
    int ticks;

    memset(&options, 0, sizeof(options));
    options.seed = 321;
    options.atoms_path = SP_DEFAULT_ATOMS_DIR;
    options.step_interval = 0.001;
    options.width = SP_DEFAULT_WIDTH;
    options.height = SP_DEFAULT_HEIGHT;
    SP_CHECK(app_init(&app, &options, error, sizeof(error)));
    SP_CHECK(app_new_run(&app));
    SP_CHECK(app.main_game.phase == GAME_PLAYING);
    SP_CHECK(app.active == &app.main_game);

    app.input_error[0] = '\0';
    app_input_clear(&app);
    SP_CHECK(!app_submit_command(&app));
    SP_CHECK(app.input_error[0] != '\0');

    snprintf(commands, sizeof(commands), "%s", "w 0");
    snprintf(app.input, sizeof(app.input), "%s", commands);
    app.input_len = (int)strlen(commands);
    SP_CHECK(!app_submit_command(&app));

    command_sequence_format(&app.main_stats.witness, commands, sizeof(commands));
    snprintf(app.input, sizeof(app.input), "%s", commands);
    app.input_len = (int)strlen(commands);
    SP_CHECK(app_submit_command(&app));
    app.step_interval = 0.0;
    for (ticks = 0; ticks < 400 && !app.in_special; ++ticks) {
        app_tick(&app, 0.01);
    }
    SP_CHECK(app.in_special);
    SP_CHECK(app.active == &app.special_game);
    SP_CHECK(app.main_game.pending_special_gate >= 0);

    flat_special(&app.special_game);
    app.active = &app.special_game;
    app.in_special = true;
    app.executing = false;
    app.paused_execution = false;
    app_input_clear(&app);
    snprintf(app.input, sizeof(app.input), "%s", "d 4");
    app.input_len = 3;
    SP_CHECK(app_submit_command(&app));
    app.step_interval = 0.0;
    for (ticks = 0; ticks < 20 && app.in_special; ++ticks) {
        app_tick(&app, 0.01);
    }
    SP_CHECK(!app.in_special);
    SP_CHECK(app.active == &app.main_game);
    SP_CHECK(app.main_game.pending_special_gate == -1);
    SP_CHECK(app.main_game.score >= 100);
    SP_CHECK(app.main_game.phase == GAME_PLAYING);

    app.main_game.phase = GAME_DEAD;
    snprintf(app.main_game.death_reason, sizeof(app.main_game.death_reason), "test death");
    app.active = &app.main_game;
    app_handle_restart(&app);
    SP_CHECK(app.main_game.phase == GAME_PLAYING);
    SP_CHECK(app.in_special == false);
    SP_CHECK(app.run_index >= 2);
    app.best_score = 0;
    app.new_best = false;
    app.main_game.score = 77;
    app_input_clear(&app);
    snprintf(app.input, sizeof(app.input), "%s", "a 1");
    app.input_len = 3;
    SP_CHECK(app_submit_command(&app));
    for (ticks = 0; ticks < 20 && app.main_game.phase != GAME_DEAD; ++ticks) {
        app_tick(&app, 0.01);
    }
    SP_CHECK(app.main_game.phase == GAME_DEAD);
    SP_CHECK(app.best_score == 77);
    SP_CHECK(app.new_best);
    app_shutdown(&app);
    return 0;
}
