#ifndef SNAKE_PUZZLE_CORE_GAME_H
#define SNAKE_PUZZLE_CORE_GAME_H

#include "snake_puzzle/core/board.h"
#include "snake_puzzle/core/event.h"
#include "snake_puzzle/core/rng.h"
#include "snake_puzzle/core/snake.h"
#include "snake_puzzle/entity/door.h"
#include "snake_puzzle/entity/food.h"
#include "snake_puzzle/entity/item.h"
#include "snake_puzzle/entity/terrain.h"

typedef struct Seal {
    int x;
    int y;
    bool collected;
} Seal;

typedef struct Orb {
    int x;
    int y;
    bool collected;
} Orb;

typedef enum GameKind {
    GAME_MAIN = 0,
    GAME_SPECIAL
} GameKind;

typedef enum GamePhase {
    GAME_PLAYING = 0,
    GAME_DEAD,
    GAME_MINI_COMPLETE
} GamePhase;

typedef enum StepResult {
    STEP_OK = 0,
    STEP_DIED,
    STEP_SPECIAL_ENTERED,
    STEP_MINI_COMPLETE,
    STEP_INVALID
} StepResult;

typedef struct Game {
    Board board;
    Snake snake;
    GameKind kind;
    GamePhase phase;
    Rng rng;
    uint64_t seed;

    int score;
    int moves;
    int foods_eaten;
    int seals_collected;
    int orbs_collected;
    int keys;
    int shields;
    int phase_tokens;
    bool double_next;

    Food foods[SP_MAX_FOODS];
    int food_count;
    Item items[SP_MAX_ITEMS];
    int item_count;
    Block blocks[SP_MAX_BLOCKS];
    int block_count;
    Gate gates[SP_MAX_GATES];
    int gate_count;
    Plate plates[SP_MAX_PLATES];
    int plate_count;
    TeleportPair teleports[SP_MAX_TELEPORTS];
    int teleport_count;
    Seal seals[SP_MAX_SEALS];
    int seal_count;
    Orb orbs[SP_MAX_ORBS];
    int orb_count;

    int pending_special_gate; /* index while a chamber transition is pending */
    bool mini_exit_open;
    char death_reason[SP_MESSAGE_MAX];
    EventQueue events;
} Game;

void game_init(Game *game, int width, int height, uint64_t seed, GameKind kind);
void game_reset_entities(Game *game);
bool game_place_snake(Game *game, Point head, Direction dir, int length);
bool game_set_cell(Game *game, int x, int y, Terrain terrain);

int game_add_food(Game *game, int x, int y, FoodKind kind);
int game_add_item(Game *game, int x, int y, ItemType type);
int game_add_block(Game *game, int x, int y);
int game_add_gate(Game *game, GateKind kind, int x, int y, int required_seals,
                  int required_orbs);
int game_add_plate(Game *game, int x, int y, uint64_t gate_mask);
int game_add_teleport(Game *game, int x1, int y1, int x2, int y2);
int game_add_seal(Game *game, int x, int y);
int game_add_orb(Game *game, int x, int y);

Food *game_food_at(Game *game, int x, int y);
Item *game_item_at(Game *game, int x, int y);
Block *game_block_at(Game *game, int x, int y);
Gate *game_gate_at(Game *game, int x, int y);

void game_open_gate(Game *game, int gate_index, const char *reason);
void game_press_plate(Game *game, int plate_index);
void game_refresh_gates(Game *game);
void game_apply_item(Game *game, ItemType type);
void game_spawn_food(Game *game, FoodKind kind);
bool game_can_spawn_at(const Game *game, int x, int y);

StepResult game_step(Game *game, Direction dir);
bool game_finish_special(Game *main_game, int bonus);
int game_special_bonus(const Game *chamber);

#endif
