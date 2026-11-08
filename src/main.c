#include "sp.h"
#include "term.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define C_RESET "\x1b[0m"

typedef enum {
    RUN_FINISHED = 0,
    RUN_RESTART,
    RUN_QUIT,
    RUN_CONTINUE
} RunResult;

static bool color_enabled = true;
static bool initial_debug = false;
static int best_score = 0;

static const char *clr(const char *code)
{
    return color_enabled ? code : "";
}

static void format_time(char *out, size_t out_size, int seconds)
{
    if (seconds < 0) {
        seconds = 0;
    }
    snprintf(out, out_size, "%02d:%02d", seconds / 60, seconds % 60);
}

static int snake_index_at(const SpGame *game, int x, int y)
{
    int i;
    for (i = 0; i < game->length; ++i) {
        if (game->snake[i].x == x && game->snake[i].y == y) {
            return i;
        }
    }
    return -1;
}

static const char *cell_color(const SpGame *game, int x, int y, unsigned char cell)
{
    bool realtime = game->mode != SP_MODE_TURN;
    int index = snake_index_at(game, x, y);

    if (index == 0) {
        return realtime ? "\x1b[1;96m" : "\x1b[1;92m";
    }
    if (index > 0) {
        return realtime ? "\x1b[36m" : "\x1b[32m";
    }
    switch (cell) {
    case SP_CELL_WALL:
        return realtime ? "\x1b[1;31m" : "\x1b[90m";
    case SP_CELL_FOOD:
        return "\x1b[1;33m";
    case SP_CELL_DOOR:
        return game_door_open(game) ? "\x1b[1;92m" : "\x1b[1;35m";
    case SP_CELL_EXIT:
        return "\x1b[1;92m";
    case SP_CELL_EMPTY:
    default:
        return "\x1b[38;5;238m";
    }
}

static char cell_glyph(const SpGame *game, int x, int y, unsigned char cell)
{
    int index = snake_index_at(game, x, y);
    if (index == 0) {
        return '@';
    }
    if (index > 0) {
        return 'o';
    }
    switch (cell) {
    case SP_CELL_WALL:
        return '#';
    case SP_CELL_FOOD:
        return '*';
    case SP_CELL_DOOR:
        return 'D';
    case SP_CELL_EXIT:
        return 'E';
    case SP_CELL_EMPTY:
    default:
        return '.';
    }
}

static void render_game(const SpGame *game, uint64_t now_ms)
{
    char time_text[16];
    char food_text[64];
    bool realtime_visual = game->mode != SP_MODE_TURN;
    int x;
    int y;

    format_time(time_text, sizeof(time_text), game_elapsed_seconds(game, now_ms));
    if (!realtime_visual) {
        const char *door = game_door_open(game)
                               ? "OPEN"
                               : (game->turn_food_eaten > game->turn_food_max ? "SEALED" : "LOCKED");
        snprintf(food_text, sizeof(food_text), "FOOD %d [%d-%d]  DOOR %s",
                 game->turn_food_eaten, game->turn_food_min, game->turn_food_max, door);
    } else {
        snprintf(food_text, sizeof(food_text), "FOOD %d  MAX LEN %d  EXIT E", game->total_food_eaten,
                 game->max_length);
    }

    fputs("\x1b[H", stdout);
    if (realtime_visual) {
        printf("%s  !!! REALTIME !!!  %s   score %d   time %s   len %d   seed %llu\x1b[K\n",
               clr("\x1b[1;41;97m"), clr(C_RESET), game_score(game, now_ms), time_text,
               game->length, (unsigned long long)game->seed);
        printf("%s  WASD steers immediately; the snake moves by itself.  %s\x1b[K\n",
               clr("\x1b[1;31m"), clr(C_RESET));
    } else {
        printf("%s  SNAKE PUZZLE // TURN MODE  %s   score %d   time %s   length %d   seed %llu\x1b[K\n",
               clr("\x1b[1;36m"), clr(C_RESET), game_score(game, now_ms), time_text,
               game->length, (unsigned long long)game->seed);
        printf("  commands: w/a/s/d [1-99]  ·  p = one BFS step  ·  auto = full BFS  ·  ? help\x1b[K\n");
    }
    printf("  %s\x1b[K\n", food_text);
    printf("  +");
    for (x = 0; x < game->width; ++x) {
        putchar('-');
    }
    printf("+\x1b[K\n");
    for (y = 0; y < game->height; ++y) {
        printf("  |");
        for (x = 0; x < game->width; ++x) {
            unsigned char cell = game->cells[y][x];
            char glyph = cell_glyph(game, x, y, cell);
            if (color_enabled) {
                printf("%s%c%s", cell_color(game, x, y, cell), glyph, C_RESET);
            } else {
                putchar(glyph);
            }
        }
        printf("|\x1b[K\n");
    }
    printf("  +");
    for (x = 0; x < game->width; ++x) {
        putchar('-');
    }
    printf("+\x1b[K\n");
    if (game->message[0] != '\0') {
        printf("  %s\x1b[K\n", game->message);
    } else {
        printf("\x1b[K\n");
    }
    if (game->debug) {
        printf("  DEBUG pos=(%d,%d) dir=%s key=0x%02x moves=%d ticks=%d elapsed=%llums\x1b[K\n",
               game->snake[0].x, game->snake[0].y, sp_dir_name(game->dir), game->last_key,
               game->moves, game->realtime_ticks,
               (unsigned long long)(now_ms >= game->start_ms ? now_ms - game->start_ms : 0));
    } else {
        printf("  v: debug HUD   r: restart   q: quit\x1b[K\n");
    }
    fputs("\x1b[J", stdout);
    fflush(stdout);
}

static void show_help(void)
{
    term_clear();
    printf("\n  SNAKE PUZZLE / FIELD NOTES\n");
    printf("  ==========================\n\n");
    printf("  TURN MODE\n");
    printf("    w/a/s/d [steps]     move; steps optional (1-99), e.g. w 3\n");
    printf("    w 2; d 4            semicolon-separated sequence\n");
    printf("    p                    one BFS autopilot step\n");
    printf("    auto                 continuous BFS autopilot\n");
    printf("    v                    toggle debug HUD\n");
    printf("    r                    restart\n");
    printf("    q                    quit\n\n");
    printf("  Map:\n");
    printf("    # wall    . floor    * food    D door    @ head    o body    E exit\n\n");
    printf("  Turn mode collisions block you and cost time. The door only opens when\n");
    printf("  the food count is inside the shown range. Overeating seals it.\n\n");
    printf("  REAL-TIME MODE\n");
    printf("    WASD steers immediately. The snake moves automatically. Wall or body\n");
    printf("    contact ends the run. Reach E to finish.\n\n");
    printf("  Score = food*10 + max_length*20 + phase bonus + time bonus.\n");
    printf("  Time bonus is 2 points per second remaining under 05:00.\n\n");
    printf("  Press Enter to return to the game.\n");
    fflush(stdout);
    {
        char line[64];
        (void)fgets(line, sizeof(line), stdin);
    }
    term_clear();
}

static bool wait_for_new_run(const SpGame *game, uint64_t now_ms)
{
    char line[64];
    char time_text[16];
    int score;
    bool new_best;

    if (game->end_ms == 0) {
        now_ms = term_now_ms();
    } else {
        now_ms = game->end_ms;
    }
    score = game_score(game, now_ms);
    new_best = score > best_score;
    if (new_best) {
        best_score = score;
    }
    format_time(time_text, sizeof(time_text), game_elapsed_seconds(game, now_ms));
    term_leave_raw();
    term_show_cursor();
    term_clear();
    printf("\n  %s  %s\n",
           game->mode == SP_MODE_WON ? "RUN COMPLETE" : "RUN ENDED",
           game->mode == SP_MODE_WON ? "The exit accepted you." : "The run was cut short.");
    printf("  ==============================================\n");
    printf("  score      %d\n", score);
    printf("  time       %s\n", time_text);
    printf("  max length %d\n", game->max_length);
    printf("  food       %d\n", game->total_food_eaten);
    printf("  seed       %llu\n", (unsigned long long)game->seed);
    printf("  best       %d%s\n", best_score, new_best ? "  <-- NEW BEST" : "");
    printf("\n  [Enter] new map     [q] quit\n");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return false;
    }
    line[strcspn(line, "\r\n")] = '\0';
    if (strcmp(line, "q") == 0 || strcmp(line, "Q") == 0 ||
        strcmp(line, "quit") == 0) {
        return false;
    }
    term_hide_cursor();
    term_clear();
    return true;
}

static RunResult play_realtime(SpGame *game)
{
    uint64_t next_tick;
    bool quit = false;

    term_enter_raw();
    term_clear();
    render_game(game, term_now_ms());
    next_tick = term_now_ms();
    while (game->mode == SP_MODE_REALTIME && !quit) {
        int key;
        uint64_t now;
        while ((key = term_read_key()) != -1) {
            int lower = tolower(key);
            SpDir dir;
            game->last_key = (unsigned char)key;
            if (lower == 'q') {
                quit = true;
                break;
            }
            if (lower == 'v') {
                game->debug = !game->debug;
                continue;
            }
            dir = sp_dir_from_char((char)key);
            if (dir != SP_DIR_NONE && dir != sp_dir_opposite(game->dir)) {
                game->dir = dir;
            }
        }
        if (quit) {
            break;
        }
        now = term_now_ms();
        if (now >= next_tick) {
            SpStep step = game_step(game, game->dir);
            render_game(game, term_now_ms());
            next_tick += 150;
            if (next_tick < now) {
                next_tick = now + 150;
            }
            if (step == SP_STEP_DEAD || step == SP_STEP_EXIT ||
                game->mode != SP_MODE_REALTIME) {
                break;
            }
        }
        term_sleep_ms(5);
    }
    term_leave_raw();
    if (!quit) {
        game->end_ms = term_now_ms();
        return RUN_FINISHED;
    }
    return RUN_QUIT;
}

static RunResult enter_realtime_and_play(SpGame *game)
{
    int i;

    term_clear();
    printf("\n\n  *** WORLD SHIFT ***\n\n");
    printf("  The door folds the maze inside out.\n");
    printf("  The world is no longer turn-based.\n");
    fflush(stdout);
    for (i = 0; i < 3; ++i) {
        term_sleep_ms(180);
        printf("  .");
        fflush(stdout);
    }
    printf("\n\n  REAL-TIME CONTROL ENGAGED\n");
    term_sleep_ms(500);
    if (!game_enter_realtime(game)) {
        term_clear();
        return RUN_CONTINUE;
    }
    return play_realtime(game);
}

static RunResult play_turn(SpGame *game)
{
    char line[256];

    while (game->mode == SP_MODE_TURN) {
        render_game(game, term_now_ms());
        printf("> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            return RUN_QUIT;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') {
            continue;
        }
        game->last_key = (unsigned char)line[0];

        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            return RUN_QUIT;
        }
        if (strcmp(line, "r") == 0 || strcmp(line, "restart") == 0) {
            return RUN_RESTART;
        }
        if (strcmp(line, "v") == 0) {
            game->debug = !game->debug;
            continue;
        }
        if (strcmp(line, "?") == 0 || strcmp(line, "h") == 0 ||
            strcmp(line, "help") == 0) {
            show_help();
            continue;
        }
        if (strcmp(line, "p") == 0 || strcmp(line, "P") == 0) {
            SpDir dir = player_move(game);
            if (dir == SP_DIR_NONE) {
                snprintf(game->message, sizeof(game->message), "BFS: no route to the next target.");
                continue;
            }
            {
                SpStep step = game_step(game, dir);
                if (step == SP_STEP_DOOR) {
                    RunResult result = enter_realtime_and_play(game);
                    if (result != RUN_CONTINUE) {
                        return result;
                    }
                } else if (step == SP_STEP_DEAD || step == SP_STEP_EXIT ||
                           game->mode != SP_MODE_TURN) {
                    game->end_ms = term_now_ms();
                    return RUN_FINISHED;
                }
            }
            continue;
        }
        if (strcmp(line, "auto") == 0) {
            while (game->mode == SP_MODE_TURN) {
                SpDir dir = player_move(game);
                SpStep step;
                if (dir == SP_DIR_NONE) {
                    snprintf(game->message, sizeof(game->message),
                             "BFS: no route to the next target.");
                    break;
                }
                step = game_step(game, dir);
                render_game(game, term_now_ms());
                term_sleep_ms(45);
                if (step == SP_STEP_DOOR) {
                    RunResult result = enter_realtime_and_play(game);
                    if (result != RUN_CONTINUE) {
                        return result;
                    }
                    break;
                }
                if (step == SP_STEP_DEAD || step == SP_STEP_EXIT ||
                    game->mode != SP_MODE_TURN) {
                    game->end_ms = term_now_ms();
                    return RUN_FINISHED;
                }
                if (step == SP_STEP_BLOCKED) {
                    break;
                }
            }
            continue;
        }

        {
            SpCommand commands[SP_MAX_COMMANDS];
            int count = 0;
            char error[128];
            int c;
            bool stop_commands = false;

            if (!game_parse_commands(line, commands, SP_MAX_COMMANDS, &count, error,
                                     sizeof(error))) {
                snprintf(game->message, sizeof(game->message), "Parse error: %s", error);
                continue;
            }
            for (c = 0; c < count && !stop_commands; ++c) {
                int s;
                for (s = 0; s < commands[c].steps; ++s) {
                    SpStep step = game_step(game, commands[c].dir);
                    if (step == SP_STEP_DOOR) {
                        RunResult result = enter_realtime_and_play(game);
                        if (result != RUN_CONTINUE) {
                            return result;
                        }
                        stop_commands = true;
                        break;
                    }
                    if (step == SP_STEP_DEAD || step == SP_STEP_EXIT ||
                        game->mode != SP_MODE_TURN) {
                        game->end_ms = term_now_ms();
                        return RUN_FINISHED;
                    }
                    if (step == SP_STEP_BLOCKED) {
                        stop_commands = true;
                        break;
                    }
                }
            }
        }
    }
    if (game->end_ms == 0) {
        game->end_ms = term_now_ms();
    }
    return RUN_FINISHED;
}

static bool start_game(SpGame *game, const char *map_path, uint64_t seed, char *error,
                       size_t error_size)
{
    if (map_path != NULL) {
        if (!game_load_map(game, map_path, error, error_size)) {
            return false;
        }
    } else if (!game_generate_turn(game, 20, 15, seed)) {
        snprintf(error, error_size, "could not generate a level");
        return false;
    }
    game->debug = initial_debug;
    game_start(game, term_now_ms());
    return true;
}

static void usage(const char *program)
{
    printf("Snake Puzzle %s — terminal turn-based + real-time snake puzzle\n\n", SP_VERSION);
    printf("Usage: %s [options] [map-file]\n\n", program);
    printf("  --seed N       deterministic random seed\n");
    printf("  --no-color     disable ANSI colors\n");
    printf("  --debug        start with debug HUD\n");
    printf("  --version      print version\n");
    printf("  --help         show this help\n\n");
    printf("Controls: w/a/s/d [steps], w 2; d 3, p = BFS step, auto = BFS run,\n");
    printf("          v = debug, r = restart, q = quit.\n");
}

static bool parse_seed(const char *text, uint64_t *out)
{
    char *end = NULL;
    unsigned long long value;

    errno = 0;
    value = strtoull(text, &end, 0);
    if (errno != 0 || end == text || (end != NULL && *end != '\0')) {
        return false;
    }
    *out = (uint64_t)value;
    return true;
}

int main(int argc, char **argv)
{
    const char *map_path = NULL;
    uint64_t base_seed = (uint64_t)time(NULL);
    int run_index = 0;
    int i;
    char error[256];

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("sp %s\n", SP_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--no-color") == 0) {
            color_enabled = false;
            continue;
        }
        if (strcmp(argv[i], "--debug") == 0) {
            initial_debug = true;
            continue;
        }
        if (strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc || !parse_seed(argv[i + 1], &base_seed)) {
                fprintf(stderr, "error: --seed expects an unsigned integer\n");
                return 2;
            }
            ++i;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            return 2;
        }
        if (map_path != NULL) {
            fprintf(stderr, "error: only one map file may be supplied\n");
            return 2;
        }
        map_path = argv[i];
    }

    term_init();
    term_hide_cursor();
    for (;;) {
        SpGame game;
        uint64_t seed = base_seed + (uint64_t)run_index * UINT64_C(0x9E3779B97F4A7C15);
        RunResult result;

        if (!start_game(&game, map_path, seed, error, sizeof(error))) {
            term_shutdown();
            fprintf(stderr, "error: %s\n", error);
            return 1;
        }
        term_clear();
        result = game.mode == SP_MODE_REALTIME ? play_realtime(&game) : play_turn(&game);
        if (result == RUN_QUIT) {
            break;
        }
        if (result == RUN_RESTART) {
            ++run_index;
            continue;
        }
        if (result == RUN_FINISHED) {
            if (game.end_ms == 0) {
                game.end_ms = term_now_ms();
            }
            if (!wait_for_new_run(&game, game.end_ms)) {
                break;
            }
            ++run_index;
            continue;
        }
        ++run_index;
    }
    term_shutdown();
    return 0;
}
