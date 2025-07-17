#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include <stdint.h>

struct ecs_handles {
    uint32_t transformable;
    uint32_t drawable;
    uint32_t render_sys;
    uint32_t move_sys;
};

#endif // GAME_CONSTANTS_H
