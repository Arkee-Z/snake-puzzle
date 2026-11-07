#include <string.h>

#include "snake_puzzle/gen/generator.h"
#include "snake_puzzle/gen/validator.h"
#include "test.h"

static bool generate(Game *game, GenStats *stats, uint64_t seed)
{
    Generator generator;
    GenOptions options;
    char error[SP_MESSAGE_MAX];

    if (!generator_init(&generator, SP_DEFAULT_ATOMS_DIR, error, sizeof(error))) {
        fprintf(stderr, "generator init failed: %s\n", error);
        return false;
    }
    memset(&options, 0, sizeof(options));
    options.width = SP_DEFAULT_WIDTH;
    options.height = SP_DEFAULT_HEIGHT;
    options.seed = seed;
    if (!generator_generate(&generator, &options, game, stats, error, sizeof(error))) {
        fprintf(stderr, "generate %llu failed: %s\n", (unsigned long long)seed, error);
        return false;
    }
    return true;
}

int test_generator_run(void)
{
    int i;
    Game previous;
    GenStats previous_stats;

    SP_CHECK(generate(&previous, &previous_stats, 42));
    for (i = 0; i < 24; ++i) {
        Game game;
        GenStats stats;
        ValidationResult validation;
        uint64_t seed = 1000 + (uint64_t)i;

        if (!generate(&game, &stats, seed)) {
            SP_CHECK(false);
            continue;
        }
        SP_CHECK(stats.valid);
        SP_CHECK(stats.core_atoms == 2);
        SP_CHECK(stats.seal_count == 2);
        SP_CHECK(stats.branch_count >= 1 && stats.branch_count <= 2);
        SP_CHECK(stats.witness_steps > 0);
        SP_CHECK(stats.witness.count > 0);
        SP_CHECK(game.snake.length == 3);
        SP_CHECK(game.snake.dir == DIR_UP);
        SP_CHECK(game.snake.body[0].x == 1 && game.snake.body[0].y == 7);
        SP_CHECK(game.board.cells[7][1].type != TERRAIN_WALL);
        SP_CHECK(game.board.cells[8][1].type != TERRAIN_WALL);
        SP_CHECK(game.board.cells[9][1].type != TERRAIN_WALL);
        SP_CHECK(validator_validate_level(&game, &stats.witness, &validation));
        SP_CHECK(validation.ok && validation.witness_solved);
        SP_CHECK(validation.witness_steps == stats.witness_steps);
        {
            int gate_index;
            int special_gates = 0;
            for (gate_index = 0; gate_index < game.gate_count; ++gate_index) {
                if (game.gates[gate_index].kind == GATE_SPECIAL) {
                    special_gates += 1;
                    SP_CHECK(!game.gates[gate_index].open);
                }
            }
            SP_CHECK_INT(special_gates, 1);
        }
    }

    {
        Game again;
        GenStats again_stats;
        SP_CHECK(generate(&again, &again_stats, 42));
        SP_CHECK_INT(again_stats.core_atoms, previous_stats.core_atoms);
        SP_CHECK(strcmp(again_stats.atoms[0], previous_stats.atoms[0]) == 0);
        SP_CHECK(strcmp(again_stats.atoms[1], previous_stats.atoms[1]) == 0);
        SP_CHECK_INT(again_stats.witness_steps, previous_stats.witness_steps);
        SP_CHECK(memcmp(&again.board, &previous.board, sizeof(Board)) == 0);
        SP_CHECK(memcmp(&again.snake, &previous.snake, sizeof(Snake)) == 0);
    }

    for (i = 0; i < 12; ++i) {
        Game chamber;
        char error[SP_MESSAGE_MAX];
        if (!generator_generate_special(5000 + (uint64_t)i, 13, 9, &chamber, error,
                                        sizeof(error))) {
            fprintf(stderr, "special generation failed: %s\n", error);
            SP_CHECK(false);
            continue;
        }
        SP_CHECK(chamber.kind == GAME_SPECIAL);
        SP_CHECK_INT(chamber.orb_count, 3);
        SP_CHECK(validator_validate_special(&chamber, error, sizeof(error)));
        SP_CHECK(chamber.snake.length == 3);
    }
    return 0;
}
