#ifndef ECS_COMPONENT_BOARD_BUTTON_H
#define ECS_COMPONENT_BOARD_BUTTON_H

#include "base/ecs/component/board.h"
#include <cglm/cglm.h>

struct board_button {
    enum player turn;
    size_t id;
    u64 background_entity;
    u64 board_entity;
    bool pressed;
    bool released;
    bool down;
    bool hovered;
};

#endif // ECS_COMPONENT_BOARD_BUTTON_H
