#include "snake_puzzle/app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SP_DEFAULT_ATOMS_DIR
#define SP_DEFAULT_ATOMS_DIR "assets/atoms"
#endif

static void copy_text(char *out, size_t out_size, const char *text)
{
    if (out_size == 0) {
        return;
    }
    if (text == NULL) {
        out[0] = '\0';
        return;
    }
    snprintf(out, out_size, "%s", text);
}

static void app_advance_execution(App *app)
{
    if (!app->executing) {
        return;
    }
    if (app->steps_left > 0) {
        app->steps_left -= 1;
    }
    if (app->steps_left == 0) {
        app->command_index += 1;
        if (app->command_index >= app->sequence.count) {
            app->executing = false;
            app->command_index = 0;
            app->steps_left = 0;
            app_add_log(app, EVENT_INFO, "Sequence complete.");
        } else {
            app->steps_left = app->sequence.commands[app->command_index].steps;
        }
    }
}

static bool app_enter_special(App *app)
{
    char error[SP_MESSAGE_MAX];
    uint64_t chamber_seed;
    int bonus;

    chamber_seed = rng_mix64(app->main_game.seed ^ UINT64_C(0xC0FFEE1234) ^
                             (uint64_t)(app->run_index * 131 + app->main_game.moves));
    if (!generator_generate_special(chamber_seed, 13, 9, &app->special_game, error,
                                    sizeof(error))) {
        app_add_log(app, EVENT_INFO, "Chamber generation failed; the gate collapses.");
        app->input_error[0] = '\0';
        copy_text(app->input_error, sizeof(app->input_error), error);
        game_finish_special(&app->main_game, 0);
        app_consume_events(app, &app->main_game);
        return false;
    }
    app->paused_execution = app->executing;
    if (app->paused_execution) {
        app->paused_sequence = app->sequence;
        app->paused_command_index = app->command_index;
        app->paused_steps_left = app->steps_left;
    }
    app->executing = false;
    command_sequence_clear(&app->sequence);
    app->command_index = 0;
    app->steps_left = 0;
    app->step_timer = 0.0;
    app->in_special = true;
    app->active = &app->special_game;
    app_input_clear(app);
    app->input_error[0] = '\0';
    app_add_log(app, EVENT_ENTER_SPECIAL, "Entered a special chamber.");
    bonus = game_special_bonus(&app->special_game);
    snprintf(app->notice, sizeof(app->notice),
             "Special chamber. Collect %d orbs, then reach the exit. Clear reward: +%d.",
             app->special_game.orb_count, bonus);
    return true;
}

static void app_complete_special(App *app)
{
    int bonus = game_special_bonus(&app->special_game);

    app->executing = false;
    app->input_error[0] = '\0';
    app_input_clear(app);
    game_finish_special(&app->main_game, bonus);
    app_consume_events(app, &app->main_game);
    app->in_special = false;
    app->active = &app->main_game;
    app_add_log(app, EVENT_SPECIAL_FINISHED, "Chamber cleared. The gate falls silent.");
    if (app->paused_execution) {
        app->sequence = app->paused_sequence;
        app->command_index = app->paused_command_index;
        app->steps_left = app->paused_steps_left;
        app->paused_execution = false;
        app->executing = true;
        app->step_timer = 0.0;
        app_add_log(app, EVENT_INFO, "Resuming the interrupted main route.");
    }
}

bool app_init(App *app, const AppOptions *options, char *error, size_t error_size)
{
    const char *path;

    memset(app, 0, sizeof(*app));
    app->base_seed = options != NULL ? options->seed : UINT64_C(0x5EED1234);
    app->step_interval = 0.105;
    app->width = SP_DEFAULT_WIDTH;
    app->height = SP_DEFAULT_HEIGHT;
    if (options != NULL) {
        if (options->step_interval > 0.0) {
            app->step_interval = options->step_interval;
        }
        if (options->width > 0) {
            app->width = sp_clamp_int(options->width, 16, SP_MAX_WIDTH);
        }
        if (options->height > 0) {
            app->height = sp_clamp_int(options->height, 11, SP_MAX_HEIGHT);
        }
    }
    path = options != NULL ? options->atoms_path : NULL;
    if (path == NULL || *path == '\0') {
        path = getenv("SNAKE_PUZZLE_ATOMS");
    }
    if (path == NULL || *path == '\0') {
        path = SP_DEFAULT_ATOMS_DIR;
    }
    copy_text(app->atoms_path, sizeof(app->atoms_path), path);
    if (!generator_init(&app->generator, path, error, error_size)) {
        return false;
    }
    app->active = &app->main_game;
    app->best_score = 0;
    return true;
}

void app_shutdown(App *app)
{
    SP_UNUSED(app);
}

bool app_new_run(App *app)
{
    GenOptions options;
    char error[SP_MESSAGE_MAX];
    uint64_t seed;

    seed = app->base_seed + (uint64_t)app->run_index * UINT64_C(0x9E3779B97F4A7C15);
    memset(&options, 0, sizeof(options));
    options.width = app->width;
    options.height = app->height;
    options.seed = seed;
    options.atoms_path = app->atoms_path;
    if (!generator_generate(&app->generator, &options, &app->main_game, &app->main_stats, error,
                            sizeof(error))) {
        copy_text(app->input_error, sizeof(app->input_error), error);
        app_add_log(app, EVENT_DEATH, error);
        return false;
    }
    app->run_index += 1;
    app->in_special = false;
    app->active = &app->main_game;
    app->executing = false;
    app->paused_execution = false;
    app->command_index = 0;
    app->steps_left = 0;
    command_sequence_clear(&app->sequence);
    command_sequence_clear(&app->paused_sequence);
    app->new_best = false;
    app->step_timer = 0.0;
    app->log_count = 0;
    app->log_head = 0;
    app->notice_type = EVENT_INFO;
    app->input_error[0] = '\0';
    snprintf(app->notice, sizeof(app->notice),
             "New map %llu. %s + %s. Collect both seals, then reach the special gate.",
             (unsigned long long)seed, app->main_stats.atoms[0], app->main_stats.atoms[1]);
    app_add_log(app, EVENT_INFO, app->notice);
    app_input_clear(app);
    return true;
}

bool app_submit_command(App *app)
{
    CommandSequence parsed;
    char error[SP_MESSAGE_MAX];
    char line[SP_LINE_MAX];

    if (app->executing) {
        copy_text(app->input_error, sizeof(app->input_error),
                  "The current sequence is still executing.");
        return false;
    }
    if (app->active == NULL || app->active->phase != GAME_PLAYING) {
        copy_text(app->input_error, sizeof(app->input_error), "The run is over.");
        return false;
    }
    if (app->input_len == 0) {
        copy_text(app->input_error, sizeof(app->input_error), "Type a command sequence first.");
        return false;
    }
    copy_text(line, sizeof(line), app->input);
    if (!command_parse(line, &parsed, error, sizeof(error))) {
        copy_text(app->input_error, sizeof(app->input_error), error);
        return false;
    }
    app->sequence = parsed;
    app->command_index = 0;
    app->steps_left = parsed.commands[0].steps;
    app->step_timer = 0.0;
    app->executing = true;
    app->input_error[0] = '\0';
    app_input_clear(app);
    {
        char formatted[SP_LINE_MAX];
        command_sequence_format(&parsed, formatted, sizeof(formatted));
        app_add_log(app, EVENT_INFO, formatted);
        snprintf(app->notice, sizeof(app->notice), "Executing %s", formatted);
    }
    return true;
}

void app_tick(App *app, double delta_time)
{
    StepResult result;

    app->elapsed += delta_time;
    if (!app->executing || app->active == NULL || app->active->phase != GAME_PLAYING) {
        return;
    }
    app->step_timer += delta_time;
    if (app->step_timer < app->step_interval) {
        return;
    }
    app->step_timer -= app->step_interval;
    if (app->command_index < 0 || app->command_index >= app->sequence.count) {
        app->executing = false;
        return;
    }
    result = game_step(app->active, app->sequence.commands[app->command_index].dir);
    app_consume_events(app, app->active);
    switch (result) {
    case STEP_DIED: {
        int final_score = app->main_game.score;
        app->executing = false;
        app->paused_execution = false;
        if (app->in_special) {
            app_add_log(app, EVENT_DEATH, "The special chamber claimed the run.");
        }
        if ((uint64_t)sp_max_int(0, final_score) > app->best_score) {
            app->best_score = (uint64_t)sp_max_int(0, final_score);
            app->new_best = true;
        }
        app->notice_type = EVENT_DEATH;
        break;
    }
    case STEP_SPECIAL_ENTERED:
        app_advance_execution(app);
        app_enter_special(app);
        break;
    case STEP_MINI_COMPLETE:
        app_advance_execution(app);
        app_complete_special(app);
        break;
    case STEP_INVALID:
        app->executing = false;
        copy_text(app->input_error, sizeof(app->input_error), "The game reached an invalid state.");
        break;
    case STEP_OK:
    default:
        app_advance_execution(app);
        break;
    }
}

void app_handle_restart(App *app)
{
    if (app->active != NULL && app->active->phase == GAME_DEAD) {
        app_new_run(app);
    }
}

void app_input_clear(App *app)
{
    app->input[0] = '\0';
    app->input_len = 0;
}

bool app_input_append(App *app, int codepoint)
{
    char c;

    if (codepoint < 0 || codepoint > 127) {
        return false;
    }
    c = (char)codepoint;
    if (c >= 'A' && c <= 'Z') {
        c = (char)(c - 'A' + 'a');
    }
    if (!((c >= 'a' && c <= 'z' && (c == 'w' || c == 'a' || c == 's' || c == 'd')) ||
          (c >= '1' && c <= '9') || c == ';' || c == ' ')) {
        return false;
    }
    if (app->input_len >= (int)sizeof(app->input) - 1) {
        return false;
    }
    app->input[app->input_len++] = c;
    app->input[app->input_len] = '\0';
    app->input_error[0] = '\0';
    return true;
}

void app_input_backspace(App *app)
{
    if (app->input_len > 0) {
        app->input_len -= 1;
        app->input[app->input_len] = '\0';
        app->input_error[0] = '\0';
    }
}

void app_add_log(App *app, EventType type, const char *text)
{
    int index;

    if (text == NULL || text[0] == '\0') {
        return;
    }
    if (app->log_count < SP_APP_LOG_LINES) {
        index = (app->log_head + app->log_count) % SP_APP_LOG_LINES;
        app->log_count += 1;
    } else {
        index = app->log_head;
        app->log_head = (app->log_head + 1) % SP_APP_LOG_LINES;
    }
    app->log[index].type = type;
    copy_text(app->log[index].text, sizeof(app->log[index].text), text);
    copy_text(app->notice, sizeof(app->notice), text);
    app->notice_type = type;
}

void app_consume_events(App *app, Game *game)
{
    Event event;

    while (event_poll(&game->events, &event)) {
        if (event.message[0] != '\0') {
            app_add_log(app, event.type, event.message);
        }
    }
}

const char *app_active_title(const App *app)
{
    if (app->in_special) {
        return "SPECIAL CHAMBER";
    }
    return "SNAKE PUZZLE";
}
