#include "snake_puzzle/entity/food.h"

const char *food_name(FoodKind kind)
{
    return kind == FOOD_GOLDEN ? "golden fruit" : "fruit";
}

char food_glyph(FoodKind kind)
{
    return kind == FOOD_GOLDEN ? '$' : '*';
}

int food_base_score(FoodKind kind)
{
    return kind == FOOD_GOLDEN ? 30 : 10;
}
