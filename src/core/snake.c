#include "snake_puzzle/core/snake.h"

#include <string.h>

void snake_clear(Snake *snake)
{
    memset(snake, 0, sizeof(*snake));
    snake->length = 0;
    snake->dir = DIR_NONE;
    snake->alive = false;
}

void snake_init(Snake *snake, Point head, Direction dir, int length)
{
    int i;

    snake_clear(snake);
    if (length < 1) {
        length = 1;
    }
    if (length > SP_MAX_SNAKE) {
        length = SP_MAX_SNAKE;
    }
    snake->body[0] = head;
    for (i = 1; i < length; ++i) {
        snake->body[i].x = head.x - dir_dx(dir) * i;
        snake->body[i].y = head.y - dir_dy(dir) * i;
    }
    snake->length = length;
    snake->dir = dir;
    snake->alive = true;
}

bool snake_occupies(const Snake *snake, int x, int y)
{
    return snake_segment_at(snake, x, y) >= 0;
}

int snake_segment_at(const Snake *snake, int x, int y)
{
    int i;

    for (i = 0; i < snake->length; ++i) {
        if (snake->body[i].x == x && snake->body[i].y == y) {
            return i;
        }
    }
    return -1;
}

Point snake_tail(const Snake *snake)
{
    Point point = {0, 0};

    if (snake->length > 0) {
        point = snake->body[snake->length - 1];
    }
    return point;
}

bool snake_move(Snake *snake, Point new_head, bool grow)
{
    int i;

    if (snake->length <= 0) {
        return false;
    }
    if (grow && snake->length < SP_MAX_SNAKE) {
        snake->length += 1;
    }
    for (i = snake->length - 1; i > 0; --i) {
        snake->body[i] = snake->body[i - 1];
    }
    snake->body[0] = new_head;
    return true;
}
