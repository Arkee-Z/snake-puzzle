#include "snake_puzzle/entity/item.h"

static const ItemDef ITEM_DEFS[ITEM_COUNT] = {
    [ITEM_KEY] = {"key", "Opens one locked door.", 'K', 5, ITEM_EFFECT_KEY},
    [ITEM_SHIELD] = {"shield", "Absorbs one fatal collision.", 'A', 5, ITEM_EFFECT_SHIELD},
    [ITEM_PHASE] = {"phase", "Dissolves the next wall you enter.", 'P', 5, ITEM_EFFECT_PHASE},
    [ITEM_DOUBLE] = {"double", "Doubles the score of the next fruit.", '2', 5, ITEM_EFFECT_DOUBLE},
};

const ItemDef *item_def(ItemType type)
{
    if (type < 0 || type >= ITEM_COUNT) {
        return &ITEM_DEFS[ITEM_KEY];
    }
    return &ITEM_DEFS[type];
}

const char *item_name(ItemType type)
{
    return item_def(type)->name;
}
