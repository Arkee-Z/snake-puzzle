#ifndef SNAKE_PUZZLE_TEST_H
#define SNAKE_PUZZLE_TEST_H

#include <stdio.h>

extern int sp_test_checks;
extern int sp_test_failures;

#define SP_CHECK(condition)                                                                    \
    do {                                                                                       \
        sp_test_checks += 1;                                                                   \
        if (!(condition)) {                                                                    \
            sp_test_failures += 1;                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);              \
        }                                                                                      \
    } while (0)

#define SP_CHECK_INT(actual, expected)                                                         \
    do {                                                                                       \
        int sp_actual = (int)(actual);                                                         \
        int sp_expected = (int)(expected);                                                     \
        sp_test_checks += 1;                                                                   \
        if (sp_actual != sp_expected) {                                                        \
            sp_test_failures += 1;                                                             \
            fprintf(stderr, "FAIL %s:%d: %s == %d, expected %d\n", __FILE__, __LINE__,        \
                    #actual, sp_actual, sp_expected);                                          \
        }                                                                                      \
    } while (0)

int test_command_run(void);
int test_snake_run(void);
int test_board_run(void);
int test_game_run(void);
int test_generator_run(void);
int test_solver_run(void);
int test_app_run(void);

#endif
