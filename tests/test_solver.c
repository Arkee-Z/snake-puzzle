#include <string.h>

#include "snake_puzzle/gen/generator.h"
#include "snake_puzzle/gen/solver.h"
#include "snake_puzzle/gen/validator.h"
#include "test.h"

int test_solver_run(void)
{
    Generator generator;
    GenOptions options;
    GenStats stats;
    Game game;
    Game copy;
    CommandSequence bad;
    SolverResult result;
    ValidationResult validation;
    char error[SP_MESSAGE_MAX];

    SP_CHECK(generator_init(&generator, SP_DEFAULT_ATOMS_DIR, error, sizeof(error)));
    memset(&options, 0, sizeof(options));
    options.width = SP_DEFAULT_WIDTH;
    options.height = SP_DEFAULT_HEIGHT;
    options.seed = 77;
    SP_CHECK(generator_generate(&generator, &options, &game, &stats, error, sizeof(error)));

    copy = game;
    SP_CHECK(solver_verify_reference(&copy, &stats.witness, &result));
    SP_CHECK(result.ok);
    SP_CHECK(result.reached_special);
    SP_CHECK(result.steps_executed == stats.witness_steps);

    command_sequence_clear(&bad);
    command_sequence_push(&bad, DIR_LEFT, 1);
    copy = game;
    SP_CHECK(!solver_verify_reference(&copy, &bad, &result));
    SP_CHECK(!result.ok);

    SP_CHECK(validator_validate_level(&game, &stats.witness, &validation));
    SP_CHECK(validation.ok);
    SP_CHECK(validation.witness_solved);

    {
        Game chamber;
        int distance = -1;
        SP_CHECK(generator_generate_special(9090, 13, 9, &chamber, error, sizeof(error)));
        SP_CHECK(validator_validate_special(&chamber, error, sizeof(error)));
        SP_CHECK(solver_all_targets_reachable(&chamber, error, sizeof(error)));
        SP_CHECK(solver_reachable(&chamber, chamber.snake.body[0],
                                  (Point){chamber.orbs[0].x, chamber.orbs[0].y}, true,
                                  &distance));
        SP_CHECK(distance > 0);
    }
    return 0;
}
