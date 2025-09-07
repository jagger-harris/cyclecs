#ifndef ECS_COMPONENT_SPRITE2D_H
#define ECS_COMPONENT_SPRITE2D_H

#include "core/util/globals.h"
#include "core/util/types.h"
#include <stdbool.h>

struct sprite2d {
    char texture_path[GLOBALS_STR_ID_MAX];
};

#endif // ECS_COMPONENT_SPRITE2D_H
