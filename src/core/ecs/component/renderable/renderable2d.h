#ifndef ECS_COMPONENT_RENDERABLE2D_H
#define ECS_COMPONENT_RENDERABLE2D_H

#include "core/util/types.h"
#include <stdbool.h>

struct renderable2d {
    bool visible;
    float opacity;
};

#endif // ECS_COMPONENT_RENDERABLE2D_H
