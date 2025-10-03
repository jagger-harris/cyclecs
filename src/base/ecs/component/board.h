#ifndef ECS_COMPONENT_BOARD_H
#define ECS_COMPONENT_BOARD_H

#include "core/util/types.h"
#include <stdbool.h>

enum player { PLAYER_NONE = 0, PLAYER_X, PLAYER_O };

struct board {
    enum player state[9];
    enum player owner;
    u64 background_entity;
    unsigned int present;
    unsigned int timeline;
    bool has_next_board;
};

#endif // ECS_COMPONENT_BOARD_H
