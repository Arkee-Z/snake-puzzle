#include "snake_puzzle/entity/door.h"

const char *gate_kind_name(GateKind kind)
{
    switch (kind) {
    case GATE_LOCKED:
        return "locked door";
    case GATE_SPECIAL:
        return "special gate";
    case GATE_MINI_EXIT:
        return "chamber exit";
    default:
        return "gate";
    }
}
