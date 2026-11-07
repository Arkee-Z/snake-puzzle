#ifndef SNAKE_PUZZLE_APP_H
#define SNAKE_PUZZLE_APP_H

#include "snake_puzzle/core/game.h"
#include "snake_puzzle/gen/generator.h"

#define SP_APP_LOG_LINES 7

typedef struct AppLogEntry {
    EventType type;
    char text[SP_MESSAGE_MAX];
} AppLogEntry;

typedef struct AppOptions {
    uint64_t seed;
    const char *atoms_path;
    double step_interval;
    int width;
    int height;
} AppOptions;

typedef struct App {
    Generator generator;
    Game main_game;
    Game special_game;
    Game *active;

    GenStats main_stats;
    bool in_special;
    bool quit;
    bool executing;
    CommandSequence sequence;
    int command_index;
    int steps_left;
    double step_timer;
    double step_interval;
    int width;
    int height;

    bool paused_execution;
    CommandSequence paused_sequence;
    int paused_command_index;
    int paused_steps_left;

    uint64_t base_seed;
    int run_index;
    uint64_t best_score;
    bool new_best;
    bool show_help;
    bool screenshot_once;
    char screenshot_path[512];
    double auto_exit_after;
    double elapsed;

    char atoms_path[SP_ATOM_PATH];
    char input[SP_LINE_MAX];
    int input_len;
    char input_error[SP_MESSAGE_MAX];
    char notice[SP_MESSAGE_MAX];
    EventType notice_type;
    AppLogEntry log[SP_APP_LOG_LINES];
    int log_count;
    int log_head;
} App;

bool app_init(App *app, const AppOptions *options, char *error, size_t error_size);
void app_shutdown(App *app);
bool app_new_run(App *app);
bool app_submit_command(App *app);
void app_tick(App *app, double delta_time);
void app_handle_restart(App *app);
void app_input_clear(App *app);
bool app_input_append(App *app, int codepoint);
void app_input_backspace(App *app);
void app_add_log(App *app, EventType type, const char *text);
void app_consume_events(App *app, Game *game);
const char *app_active_title(const App *app);

#endif
