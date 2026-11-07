#ifndef SNAKE_PUZZLE_ENTITY_ITEM_H
#define SNAKE_PUZZLE_ENTITY_ITEM_H

#include "snake_puzzle/common.h"

typedef enum ItemType {
    ITEM_KEY = 0,
    ITEM_SHIELD,
    ITEM_PHASE,
    ITEM_DOUBLE,
    ITEM_COUNT
} ItemType;

typedef enum ItemEffect {
    ITEM_EFFECT_KEY = 0,
    ITEM_EFFECT_SHIELD,
    ITEM_EFFECT_PHASE,
    ITEM_EFFECT_DOUBLE
} ItemEffect;

typedef struct Item {
    int x;
    int y;
    ItemType type;
    bool active;
} Item;

typedef struct ItemDef {
    const char *name;
    const char *description;
    char glyph;
    int score;
    ItemEffect effect;
} ItemDef;

const ItemDef *item_def(ItemType type);
const char *item_name(ItemType type);

#endif
