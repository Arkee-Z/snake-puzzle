#ifndef SNAKE_PUZZLE_ENTITY_FOOD_H
#define SNAKE_PUZZLE_ENTITY_FOOD_H

#include "snake_puzzle/common.h"

typedef enum FoodKind {
    FOOD_NORMAL = 0,
    FOOD_GOLDEN
} FoodKind;

typedef struct Food {
    int x;
    int y;
    FoodKind kind;
    bool active;
} Food;

const char *food_name(FoodKind kind);
char food_glyph(FoodKind kind);
int food_base_score(FoodKind kind);

#endif
