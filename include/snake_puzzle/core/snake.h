#ifndef SNAKE_PUZZLE_CORE_SNAKE_H
#define SNAKE_PUZZLE_CORE_SNAKE_H

#include "snake_puzzle/common.h"

typedef struct Snake {
    Point body[SP_MAX_SNAKE];
    int length;
    Direction dir;
    bool alive;
} Snake;

void snake_init(Snake *snake, Point head, Direction dir, int length);
void snake_clear(Snake *snake);
bool snake_occupies(const Snake *snake, int x, int y);
int snake_segment_at(const Snake *snake, int x, int y);
Point snake_tail(const Snake *snake);
bool snake_move(Snake *snake, Point new_head, bool grow);

#endif
