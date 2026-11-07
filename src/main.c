#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "snake_puzzle/app.h"
#include "snake_puzzle/gen/generator.h"
#include "snake_puzzle/gen/validator.h"
#include "snake_puzzle/ui/render_cli.h"
#include "snake_puzzle/ui/render_raylib.h"

#ifndef SP_DEFAULT_ATOMS_DIR
#define SP_DEFAULT_ATOMS_DIR "assets/atoms"
#endif

typedef struct CliOptions {
    uint64_t seed;
    const char *atoms_path;
    int batch_count;
    int validate;
    int dump;
    const char *dump_path;
    const char *screenshot_path;
    double auto_exit;
    int step_ms;
    int width;
    int height;
    int show_help;
    int demo_special;
    int demo_death;
} CliOptions;

static void print_usage(FILE *out)
{
    fprintf(out,
            "Snake Puzzle — command-driven roguelike grid logic\n"
            "\n"
            "Usage: snake_puzzle [options]\n"
            "\n"
            "  --seed N            deterministic base seed (default: time-based)\n"
            "  --atoms PATH        atom directory or exported .spa file\n"
            "  --width W           main board width (default 20, max %d)\n"
            "  --height H          main board height (default 15, max %d)\n"
            "  --step-ms N         milliseconds between executed steps (default 105)\n"
            "  --auto-exit SEC     close the graphical window after SEC seconds\n"
            "  --screenshot PATH   save one rendered frame and exit\n"
            "  --show-help         open the field manual on launch\n"
            "  --demo-special      open a generated special chamber (visual QA)\n"
            "  --demo-death        show the run-over screen (visual QA)\n"
            "\n"
            "Headless tooling:\n"
            "  --batch N           generate N levels and print JSON Lines\n"
            "  --validate-seed N   generate and validate one level\n"
            "  --dump PATH         write a validated map dump ('-' for stdout)\n"
            "  --list-atoms        print the loaded atom registry\n"
            "\n"
            "  -h, --help          show this help\n"
            "  -v, --version       show version\n"
            "\n"
            "Examples:\n"
            "  snake_puzzle --seed 42\n"
            "  snake_puzzle --seed 42 --screenshot build/frame.png\n"
            "  snake_puzzle --batch 200 --seed 1000\n"
            "  snake_puzzle --validate-seed 42 --dump build/map.txt\n",
            SP_MAX_WIDTH, SP_MAX_HEIGHT);
}

static bool parse_u64(const char *text, uint64_t *out)
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

static bool parse_int(const char *text, int *out)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || (end != NULL && *end != '\0') || value < 0 ||
        value > 1000000) {
        return false;
    }
    *out = (int)value;
    return true;
}

static bool parse_double(const char *text, double *out)
{
    char *end = NULL;
    double value;

    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || (end != NULL && *end != '\0') || value < 0.0) {
        return false;
    }
    *out = value;
    return true;
}

static const char *find_option_value(int argc, char **argv, int *index)
{
    if (*index + 1 >= argc) {
        return NULL;
    }
    *index += 1;
    return argv[*index];
}

static bool parse_args(int argc, char **argv, CliOptions *options)
{
    int i;

    memset(options, 0, sizeof(*options));
    options->atoms_path = getenv("SNAKE_PUZZLE_ATOMS");
    if (options->atoms_path == NULL || options->atoms_path[0] == '\0') {
        options->atoms_path = SP_DEFAULT_ATOMS_DIR;
    }
    options->width = SP_DEFAULT_WIDTH;
    options->height = SP_DEFAULT_HEIGHT;
    options->step_ms = 105;
    options->seed = (uint64_t)time(NULL) ^ ((uint64_t)getpid() << 32);

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(stdout);
            exit(0);
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            printf("snake_puzzle 1.0.0\n");
            exit(0);
        } else if (strcmp(arg, "--seed") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_u64(value, &options->seed)) {
                fprintf(stderr, "error: --seed expects an unsigned integer\n");
                return false;
            }
        } else if (strcmp(arg, "--atoms") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL) {
                fprintf(stderr, "error: --atoms expects a path\n");
                return false;
            }
            options->atoms_path = value;
        } else if (strcmp(arg, "--width") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_int(value, &options->width)) {
                fprintf(stderr, "error: --width expects a positive integer\n");
                return false;
            }
        } else if (strcmp(arg, "--height") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_int(value, &options->height)) {
                fprintf(stderr, "error: --height expects a positive integer\n");
                return false;
            }
        } else if (strcmp(arg, "--step-ms") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_int(value, &options->step_ms)) {
                fprintf(stderr, "error: --step-ms expects a non-negative integer\n");
                return false;
            }
        } else if (strcmp(arg, "--batch") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_int(value, &options->batch_count) ||
                options->batch_count <= 0) {
                fprintf(stderr, "error: --batch expects a positive count\n");
                return false;
            }
        } else if (strcmp(arg, "--validate-seed") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_u64(value, &options->seed)) {
                fprintf(stderr, "error: --validate-seed expects an unsigned integer\n");
                return false;
            }
            options->validate = 1;
        } else if (strcmp(arg, "--dump") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL) {
                fprintf(stderr, "error: --dump expects a path\n");
                return false;
            }
            options->dump = 1;
            options->dump_path = value;
            if (!options->validate) {
                options->validate = 1;
            }
        } else if (strcmp(arg, "--screenshot") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL) {
                fprintf(stderr, "error: --screenshot expects a path\n");
                return false;
            }
            options->screenshot_path = value;
        } else if (strcmp(arg, "--auto-exit") == 0) {
            const char *value = find_option_value(argc, argv, &i);
            if (value == NULL || !parse_double(value, &options->auto_exit)) {
                fprintf(stderr, "error: --auto-exit expects seconds\n");
                return false;
            }
        } else if (strcmp(arg, "--show-help") == 0) {
            options->show_help = 1;
        } else if (strcmp(arg, "--demo-special") == 0) {
            options->demo_special = 1;
        } else if (strcmp(arg, "--demo-death") == 0) {
            options->demo_death = 1;
        } else if (strcmp(arg, "--list-atoms") == 0) {
            Generator generator;
            char error[SP_MESSAGE_MAX];
            int a;
            if (!generator_init(&generator, options->atoms_path, error, sizeof(error))) {
                fprintf(stderr, "error: %s\n", error);
                return false;
            }
            for (a = 0; a < generator.library.count; ++a) {
                const Atom *atom = &generator.library.atoms[a];
                printf("%-20s %dx%d difficulty=%d weight=%d solution=%d steps\n",
                       atom->name, atom->width, atom->height, atom->difficulty,
                       atom->weight, command_sequence_total_steps(&atom->solution));
            }
            exit(0);
        } else {
            fprintf(stderr, "error: unknown option '%s' (try --help)\n", arg);
            return false;
        }
    }
    if (options->width < 16 || options->width > SP_MAX_WIDTH || options->height < 11 ||
        options->height > SP_MAX_HEIGHT) {
        fprintf(stderr, "error: board must be 16..%d wide and 11..%d tall\n", SP_MAX_WIDTH,
                SP_MAX_HEIGHT);
        return false;
    }
    if (options->step_ms > 10000) {
        options->step_ms = 10000;
    }
    if (options->screenshot_path != NULL && options->auto_exit <= 0.0) {
        options->auto_exit = 1.0;
    }
    return true;
}

static void json_escape(FILE *out, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;

    fputc('"', out);
    while (p != NULL && *p != '\0') {
        switch (*p) {
        case '\\':
            fputs("\\\\", out);
            break;
        case '"':
            fputs("\\\"", out);
            break;
        case '\n':
            fputs("\\n", out);
            break;
        case '\r':
            fputs("\\r", out);
            break;
        case '\t':
            fputs("\\t", out);
            break;
        default:
            if (*p < 0x20) {
                fprintf(out, "\\u%04x", (unsigned int)*p);
            } else {
                fputc((int)*p, out);
            }
            break;
        }
        ++p;
    }
    fputc('"', out);
}

static int run_batch(const CliOptions *options)
{
    Generator generator;
    char error[SP_MESSAGE_MAX];
    int index;

    if (!generator_init(&generator, options->atoms_path, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }
    for (index = 0; index < options->batch_count; ++index) {
        GenOptions gen_options;
        Game game;
        GenStats stats;
        uint64_t seed = options->seed + (uint64_t)index;

        memset(&gen_options, 0, sizeof(gen_options));
        gen_options.width = options->width;
        gen_options.height = options->height;
        gen_options.seed = seed;
        gen_options.atoms_path = options->atoms_path;
        if (generator_generate(&generator, &gen_options, &game, &stats, error, sizeof(error))) {
            printf("{\"index\":%d,\"seed\":%llu,\"valid\":true,\"attempts\":%d,"
                   "\"atoms\":[\"%s\",\"%s\"],\"seals\":%d,\"branches\":%d,"
                   "\"branch_cells\":%d,\"density\":%.6f,\"witness_steps\":%d,"
                   "\"witness_commands\":%d,\"chain_length\":%d,\"width\":%d,"
                   "\"height\":%d}\n",
                   index, (unsigned long long)seed, stats.attempts, stats.atoms[0],
                   stats.atoms[1], stats.seal_count, stats.branch_count, stats.branch_cells,
                   stats.floor_density, stats.witness_steps, stats.witness_commands,
                   stats.chain_length, options->width, options->height);
        } else {
            printf("{\"index\":%d,\"seed\":%llu,\"valid\":false,\"error\":",
                   index, (unsigned long long)seed);
            json_escape(stdout, error);
            printf(",\"width\":%d,\"height\":%d}\n", options->width, options->height);
        }
        fflush(stdout);
    }
    return 0;
}

static int run_validate(const CliOptions *options)
{
    Generator generator;
    GenOptions gen_options;
    GenStats stats;
    Game game;
    ValidationResult validation;
    char error[SP_MESSAGE_MAX];
    FILE *dump = NULL;

    if (!generator_init(&generator, options->atoms_path, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }
    memset(&gen_options, 0, sizeof(gen_options));
    gen_options.width = options->width;
    gen_options.height = options->height;
    gen_options.seed = options->seed;
    gen_options.atoms_path = options->atoms_path;
    if (!generator_generate(&generator, &gen_options, &game, &stats, error, sizeof(error))) {
        fprintf(stderr, "INVALID seed=%llu: %s\n", (unsigned long long)options->seed, error);
        return 1;
    }
    if (!validator_validate_level(&game, &stats.witness, &validation)) {
        fprintf(stderr, "INVALID seed=%llu: %s\n", (unsigned long long)options->seed,
                validation.error);
        return 1;
    }
    printf("VALID seed=%llu attempts=%d atoms=%s+%s seals=%d branches=%d "
           "witness_steps=%d witness_commands=%d density=%.3f\n",
           (unsigned long long)options->seed, stats.attempts, stats.atoms[0], stats.atoms[1],
           stats.seal_count, stats.branch_count, stats.witness_steps,
           stats.witness_commands, stats.floor_density);
    if (options->dump) {
        if (strcmp(options->dump_path, "-") == 0) {
            render_map_dump(&game, &stats, stdout);
        } else {
            dump = fopen(options->dump_path, "wb");
            if (dump == NULL) {
                fprintf(stderr, "error: cannot write %s\n", options->dump_path);
                return 1;
            }
            render_map_dump(&game, &stats, dump);
            fclose(dump);
            printf("map dump written to %s\n", options->dump_path);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    CliOptions options;
    App app;
    AppOptions app_options;
    char error[SP_MESSAGE_MAX];

    if (!parse_args(argc, argv, &options)) {
        return 2;
    }
    if (options.batch_count > 0) {
        return run_batch(&options);
    }
    if (options.validate) {
        return run_validate(&options);
    }
    memset(&app_options, 0, sizeof(app_options));
    app_options.seed = options.seed;
    app_options.atoms_path = options.atoms_path;
    app_options.step_interval = (double)options.step_ms / 1000.0;
    app_options.width = options.width;
    app_options.height = options.height;
    if (!app_init(&app, &app_options, error, sizeof(error))) {
        fprintf(stderr, "error: %s\n", error);
        return 1;
    }
    if (!app_new_run(&app)) {
        fprintf(stderr, "error: %s\n", app.input_error[0] != '\0' ? app.input_error
                                                                 : "could not generate level");
        app_shutdown(&app);
        return 1;
    }
    if (options.demo_special) {
        if (!generator_generate_special(app.base_seed ^ UINT64_C(0xC0FFEE), 13, 9,
                                        &app.special_game, error, sizeof(error))) {
            fprintf(stderr, "error: %s\n", error);
            app_shutdown(&app);
            return 1;
        }
        app.in_special = true;
        app.active = &app.special_game;
        app.executing = false;
        app.paused_execution = false;
        app_add_log(&app, EVENT_ENTER_SPECIAL, "Previewing a special chamber.");
    }
    if (options.demo_death) {
        app.active = &app.main_game;
        app.main_game.phase = GAME_DEAD;
        snprintf(app.main_game.death_reason, sizeof(app.main_game.death_reason),
                 "You hit a wall at the edge of the grid.");
        app.main_game.score = 245;
        app.new_best = true;
        app.best_score = 245;
    }
    app.auto_exit_after = options.auto_exit;
    app.show_help = options.show_help != 0;
    if (options.screenshot_path != NULL) {
        snprintf(app.screenshot_path, sizeof(app.screenshot_path), "%s",
                 options.screenshot_path);
        app.screenshot_once = true;
    }
    (void)raylib_run_app(&app);
    app_shutdown(&app);
    return 0;
}
