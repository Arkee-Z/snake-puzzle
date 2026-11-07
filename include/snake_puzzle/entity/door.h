#ifndef SNAKE_PUZZLE_ENTITY_DOOR_H
#define SNAKE_PUZZLE_ENTITY_DOOR_H

#include "snake_puzzle/common.h"

typedef enum GateKind {
    GATE_LOCKED = 0,
    GATE_SPECIAL,
    GATE_MINI_EXIT
} GateKind;

typedef struct Gate {
    GateKind kind;
    int x;
    int y;
    bool open;
    bool used;
    int required_seals;
    int required_orbs;
} Gate;

const char *gate_kind_name(GateKind kind);

#endif
