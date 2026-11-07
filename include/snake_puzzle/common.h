#ifndef SNAKE_PUZZLE_COMMON_H
#define SNAKE_PUZZLE_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SP_DEFAULT_WIDTH 20
#define SP_DEFAULT_HEIGHT 15
#define SP_MAX_WIDTH 32
#define SP_MAX_HEIGHT 24

#define SP_MAX_SNAKE 256
#define SP_MAX_COMMANDS 32
#define SP_MAX_EVENTS 256
#define SP_MAX_FOODS 8
#define SP_MAX_ITEMS 16
#define SP_MAX_BLOCKS 24
#define SP_MAX_GATES 32
#define SP_MAX_PLATES 24
#define SP_MAX_TELEPORTS 16
#define SP_MAX_SEALS 8
#define SP_MAX_ORBS 8

#define SP_ATOM_MAX 24
#define SP_ATOM_MAX_W 10
#define SP_ATOM_MAX_H 10
#define SP_ATOM_NAME 48
#define SP_ATOM_PATH 300

#define SP_LINE_MAX 1024
#define SP_MESSAGE_MAX 192

#define SP_UNUSED(x) ((void)(x))

typedef enum Direction {
    DIR_UP = 0,
    DIR_RIGHT = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3,
    DIR_NONE = -1
} Direction;

typedef struct Point {
    int x;
    int y;
} Point;

static inline int dir_dx(Direction dir)
{
    switch (dir) {
    case DIR_UP:
        return 0;
    case DIR_RIGHT:
        return 1;
    case DIR_DOWN:
        return 0;
    case DIR_LEFT:
        return -1;
    default:
        return 0;
    }
}

static inline int dir_dy(Direction dir)
{
    switch (dir) {
    case DIR_UP:
        return -1;
    case DIR_RIGHT:
        return 0;
    case DIR_DOWN:
        return 1;
    case DIR_LEFT:
        return 0;
    default:
        return 0;
    }
}

static inline Direction dir_opposite(Direction dir)
{
    switch (dir) {
    case DIR_UP:
        return DIR_DOWN;
    case DIR_RIGHT:
        return DIR_LEFT;
    case DIR_DOWN:
        return DIR_UP;
    case DIR_LEFT:
        return DIR_RIGHT;
    default:
        return DIR_NONE;
    }
}

static inline Direction dir_from_char(char c)
{
    switch (c) {
    case 'w':
    case 'W':
        return DIR_UP;
    case 'd':
    case 'D':
        return DIR_RIGHT;
    case 's':
    case 'S':
        return DIR_DOWN;
    case 'a':
    case 'A':
        return DIR_LEFT;
    default:
        return DIR_NONE;
    }
}

static inline char dir_to_char(Direction dir)
{
    switch (dir) {
    case DIR_UP:
        return 'w';
    case DIR_RIGHT:
        return 'd';
    case DIR_DOWN:
        return 's';
    case DIR_LEFT:
        return 'a';
    default:
        return '?';
    }
}

static inline const char *dir_name(Direction dir)
{
    switch (dir) {
    case DIR_UP:
        return "up";
    case DIR_RIGHT:
        return "right";
    case DIR_DOWN:
        return "down";
    case DIR_LEFT:
        return "left";
    default:
        return "none";
    }
}

static inline int sp_min_int(int a, int b)
{
    return a < b ? a : b;
}

static inline int sp_max_int(int a, int b)
{
    return a > b ? a : b;
}

static inline int sp_clamp_int(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

#endif
