#include "test.h"

int sp_test_checks = 0;
int sp_test_failures = 0;

int main(void)
{
    test_command_run();
    test_snake_run();
    test_board_run();
    test_game_run();
    test_generator_run();
    test_solver_run();
    test_app_run();

    printf("%d checks, %d failures\n", sp_test_checks, sp_test_failures);
    return sp_test_failures == 0 ? 0 : 1;
}
