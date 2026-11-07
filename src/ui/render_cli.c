#include "snake_puzzle/ui/render_cli.h"

#include <stdio.h>
#include <string.h>

#include "snake_puzzle/entity/item.h"

static const char *ansi_color(const Game *game, int x, int y)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    const Snake *snake = &game->snake;

    if (snake->length > 0 && snake->body[0].x == x && snake->body[0].y == y) {
        return "\033[1;92m";
    }
    if (snake_occupies(snake, x, y)) {
        return "\033[1;32m";
    }
    if (game_block_at((Game *)game, x, y) != NULL) {
        return "\033[1;33m";
    }
    if (game_food_at((Game *)game, x, y) != NULL) {
        return "\033[1;91m";
    }
    if (game_item_at((Game *)game, x, y) != NULL) {
        return "\033[1;96m";
    }
    switch (cell->type) {
    case TERRAIN_WALL:
        return "\033[90m";
    case TERRAIN_SPECIAL_GATE:
        return "\033[1;95m";
    case TERRAIN_LOCKED:
        return "\033[1;35m";
    case TERRAIN_PLATE:
        return "\033[36m";
    case TERRAIN_BREAK:
        return "\033[1;33m";
    case TERRAIN_PIT:
        return "\033[31m";
    case TERRAIN_SEAL:
        return "\033[1;95m";
    case TERRAIN_TELEPORT:
        return "\033[1;94m";
    case TERRAIN_ORB:
        return "\033[1;96m";
    case TERRAIN_ONEWAY:
        return "\033[1;34m";
    case TERRAIN_MINI_EXIT:
        return "\033[1;93m";
    default:
        return "\033[0m";
    }
}

char render_cli_cell(const Game *game, int x, int y)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    Food *food;
    Item *item;
    const Gate *gate;
    int i;

    if (cell == NULL) {
        return ' ';
    }
    if (game->snake.length > 0 && game->snake.body[0].x == x && game->snake.body[0].y == y) {
        return '@';
    }
    for (i = 1; i < game->snake.length; ++i) {
        if (game->snake.body[i].x == x && game->snake.body[i].y == y) {
            return 'o';
        }
    }
    if (game_block_at((Game *)game, x, y) != NULL) {
        return 'B';
    }
    food = game_food_at((Game *)game, x, y);
    if (food != NULL) {
        return food_glyph(food->kind);
    }
    item = game_item_at((Game *)game, x, y);
    if (item != NULL) {
        return item_def(item->type)->glyph;
    }
    switch (cell->type) {
    case TERRAIN_WALL:
        return '#';
    case TERRAIN_FLOOR:
        return '.';
    case TERRAIN_ONEWAY:
        switch ((Direction)cell->oneway_dir) {
        case DIR_UP:
            return '^';
        case DIR_RIGHT:
            return '>';
        case DIR_DOWN:
            return 'v';
        case DIR_LEFT:
        default:
            return '<';
        }
    case TERRAIN_LOCKED:
        gate = cell->gate >= 0 && cell->gate < game->gate_count ? &game->gates[cell->gate]
                                                               : NULL;
        return gate != NULL && gate->open ? '+' : 'L';
    case TERRAIN_SPECIAL_GATE:
        return 'D';
    case TERRAIN_BREAK:
        return '~';
    case TERRAIN_PIT:
        return 'x';
    case TERRAIN_PLATE:
        return cell->plate >= 0 && cell->plate < game->plate_count &&
                       game->plates[cell->plate].pressed
                   ? '='
                   : '_';
    case TERRAIN_SEAL:
        return cell->seal >= 0 && cell->seal < game->seal_count &&
                       game->seals[cell->seal].collected
                   ? 's'
                   : 'S';
    case TERRAIN_TELEPORT:
        return 'T';
    case TERRAIN_MINI_EXIT:
        gate = cell->gate >= 0 && cell->gate < game->gate_count ? &game->gates[cell->gate]
                                                               : NULL;
        return gate != NULL && gate->open ? 'E' : 'e';
    case TERRAIN_ORB:
        return cell->orb >= 0 && cell->orb < game->orb_count &&
                       game->orbs[cell->orb].collected
                   ? ','
                   : 'O';
    default:
        return '?';
    }
}

void render_cli_legend(FILE *out, bool color)
{
    if (color) {
        fprintf(out, "\033[1;92m@\033[0m head  \033[1;32mo\033[0m body  "
                     "\033[90m#\033[0m wall  \033[1;91m*\033[0m fruit  "
                     "\033[1;95mS\033[0m seal  \033[1;95mD\033[0m gate  "
                     "\033[1;35mL\033[0m door  \033[36m_\033[0m plate\n");
        fprintf(out, "\033[1;34m<^v>\033[0m one-way  \033[1;33mB\033[0m block  "
                     "\033[1;33m~\033[0m fracture  \033[1;94mT\033[0m teleport  "
                     "\033[1;96mK A P 2\033[0m items\n");
    } else {
        fprintf(out, "@ head  o body  # wall  * fruit  S seal  D gate  L door  _ plate\n");
        fprintf(out, "<^v> one-way  B block  ~ fracture  T teleport  K A P 2 items\n");
    }
}

void render_cli_game(const Game *game, FILE *out, bool color)
{
    int x;
    int y;
    bool special = game->kind == GAME_SPECIAL;

    fprintf(out, "%s  seed=%llu  score=%d  length=%d  moves=%d\n",
            special ? "SPECIAL CHAMBER" : "SNAKE PUZZLE",
            (unsigned long long)game->seed, game->score, game->snake.length, game->moves);
    if (special) {
        fprintf(out, "orbs %d/%d  exit %s\n", game->orbs_collected, game->orb_count,
                game->mini_exit_open ? "OPEN" : "SEALED");
    } else {
        fprintf(out, "seals %d/%d  keys=%d  shields=%d  phase=%d  double=%s\n",
                game->seals_collected, game->seal_count, game->keys, game->shields,
                game->phase_tokens, game->double_next ? "yes" : "no");
    }
    for (y = 0; y < game->board.height; ++y) {
        for (x = 0; x < game->board.width; ++x) {
            char ch = render_cli_cell(game, x, y);
            if (color) {
                fprintf(out, "%s%c\033[0m", ansi_color(game, x, y), ch);
            } else {
                fputc(ch, out);
            }
        }
        fputc('\n', out);
    }
    render_cli_legend(out, color);
    if (game->phase == GAME_DEAD) {
        fprintf(out, "\nGAME OVER: %s\n", game->death_reason);
    } else if (special) {
        fprintf(out, "\nCollect every orb, then reach the chamber exit.\n");
    } else {
        fprintf(out, "\nCollect all seals, then enter the special gate.\n");
    }
}

void render_map_dump(const Game *game, const GenStats *stats, FILE *out)
{
    int x;
    int y;
    int i;

    fprintf(out, "# Snake Puzzle map dump v1\n");
    fprintf(out, "seed %llu\n", (unsigned long long)(stats != NULL ? stats->seed : game->seed));
    fprintf(out, "size %d %d\n", game->board.width, game->board.height);
    if (stats != NULL) {
        fprintf(out, "atoms %d\n", stats->core_atoms);
        for (i = 0; i < stats->core_atoms && i < SP_GEN_MAX_ATOMS; ++i) {
            fprintf(out, "atom %d %s %d %d %d %d\n", i + 1, stats->placements[i].name,
                    stats->placements[i].x, stats->placements[i].y,
                    stats->placements[i].width, stats->placements[i].height);
        }
        fprintf(out, "chain_length %d\n", stats->chain_length);
        fprintf(out, "seals %d\n", stats->seal_count);
        fprintf(out, "branches %d\n", stats->branch_count);
        fprintf(out, "branch_cells %d\n", stats->branch_cells);
        fprintf(out, "witness_steps %d\n", stats->witness_steps);
        fprintf(out, "witness_commands %d\n", stats->witness_commands);
        fprintf(out, "registry ");
        for (i = 0; i < stats->core_atoms; ++i) {
            fprintf(out, "%s%s", i == 0 ? "" : ",", stats->atoms[i]);
        }
        fputc('\n', out);
        fprintf(out, "valid %d\n", stats->valid ? 1 : 0);
    }
    fprintf(out, "snake %d %d %d %d\n", game->snake.body[0].x, game->snake.body[0].y,
            game->snake.length, (int)game->snake.dir);
    fprintf(out, "map\n");
    for (y = 0; y < game->board.height; ++y) {
        for (x = 0; x < game->board.width; ++x) {
            fputc(render_cli_cell(game, x, y), out);
        }
        fputc('\n', out);
    }
    fprintf(out, "end\n");
}
