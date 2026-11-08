#ifndef SP_H
#define SP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SP_MAP_W 40
#define SP_MAP_H 24
#define SP_SNAKE_MAX 100
#define SP_MAX_COMMANDS 32
#define SP_MAX_STEPS 99
#define SP_MESSAGE_MAX 192
#define SP_VERSION "0.5.1"

typedef enum {
    SP_DIR_UP = 0,
    SP_DIR_RIGHT,
    SP_DIR_DOWN,
    SP_DIR_LEFT,
    SP_DIR_NONE
} SpDir;

typedef enum {
    SP_CELL_EMPTY = 0,
    SP_CELL_WALL,
    SP_CELL_FOOD,
    SP_CELL_DOOR,
    SP_CELL_EXIT
} SpCell;

typedef enum {
    SP_MODE_TURN = 0,
    SP_MODE_REALTIME,
    SP_MODE_WON,
    SP_MODE_DEAD
} SpMode;

typedef enum {
    SP_STEP_OK = 0,
    SP_STEP_BLOCKED,
    SP_STEP_DOOR,
    SP_STEP_EXIT,
    SP_STEP_DEAD
} SpStep;

typedef struct {
    int x;
    int y;
} SpPos;

typedef struct {
    SpDir dir;
    int steps;
} SpCommand;

typedef struct {
    int width;
    int height;
    unsigned char cells[SP_MAP_H][SP_MAP_W];
    SpPos snake[SP_SNAKE_MAX];
    int length;
    SpDir dir;
    SpMode mode;

    int turn_food_total;
    int turn_food_eaten;
    int turn_food_min;
    int turn_food_max;
    int total_food_eaten;
    int max_length;

    int door_x;
    int door_y;
    int exit_x;
    int exit_y;

    int phase_bonus;
    int moves;
    int realtime_ticks;
    uint64_t seed;
    uint64_t start_ms;
    uint64_t end_ms;

    bool debug;
    unsigned char last_key;
    char message[SP_MESSAGE_MAX];
} SpGame;

void game_new(SpGame *game);
void game_start(SpGame *game, uint64_t now_ms);
bool game_load_map(SpGame *game, const char *path, char *error, size_t error_size);
bool game_generate_turn(SpGame *game, int width, int height, uint64_t seed);
bool game_enter_realtime(SpGame *game);
SpStep game_step(SpGame *game, SpDir dir);

bool game_parse_commands(const char *text, SpCommand *out, int max_commands, int *out_count,
                         char *error, size_t error_size);
bool game_door_open(const SpGame *game);
int game_score(const SpGame *game, uint64_t now_ms);
int game_elapsed_seconds(const SpGame *game, uint64_t now_ms);
int game_time_bonus(const SpGame *game, uint64_t now_ms);

SpDir bfs_next_direction(const SpGame *game);
SpDir player_move(const SpGame *game);

const char *sp_dir_name(SpDir dir);
SpDir sp_dir_from_char(char c);
SpDir sp_dir_opposite(SpDir dir);

#endif
