#include "snake_puzzle/core/snake.h"
#include "test.h"

int test_snake_run(void)
{
    Snake snake;
    Point head = {5, 5};

    snake_init(&snake, head, DIR_UP, 3);
    SP_CHECK_INT(snake.length, 3);
    SP_CHECK_INT(snake.body[0].x, 5);
    SP_CHECK_INT(snake.body[0].y, 5);
    SP_CHECK_INT(snake.body[1].y, 6);
    SP_CHECK_INT(snake.body[2].y, 7);
    SP_CHECK(snake_occupies(&snake, 5, 6));
    SP_CHECK(!snake_occupies(&snake, 5, 4));
    SP_CHECK_INT(snake_segment_at(&snake, 5, 7), 2);

    SP_CHECK(snake_move(&snake, (Point){5, 4}, false));
    SP_CHECK_INT(snake.length, 3);
    SP_CHECK_INT(snake.body[0].y, 4);
    SP_CHECK_INT(snake.body[1].y, 5);
    SP_CHECK_INT(snake.body[2].y, 6);

    SP_CHECK(snake_move(&snake, (Point){6, 4}, true));
    SP_CHECK_INT(snake.length, 4);
    SP_CHECK_INT(snake.body[3].y, 6);

    snake_init(&snake, (Point){2, 2}, DIR_RIGHT, 1);
    SP_CHECK_INT(snake.length, 1);
    SP_CHECK_INT(snake.body[0].x, 2);
    SP_CHECK_INT(snake_tail(&snake).x, 2);
    return 0;
}
