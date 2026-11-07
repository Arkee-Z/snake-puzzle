#ifndef SNAKE_PUZZLE_UI_RENDER_CLI_H
#define SNAKE_PUZZLE_UI_RENDER_CLI_H

#include <stdbool.h>
#include <stdio.h>

#include "snake_puzzle/core/game.h"
#include "snake_puzzle/gen/generator.h"

void render_cli_game(const Game *game, FILE *out, bool color);
char render_cli_cell(const Game *game, int x, int y);
void render_cli_legend(FILE *out, bool color);
void render_map_dump(const Game *game, const struct GenStats *stats, FILE *out);

#endif
