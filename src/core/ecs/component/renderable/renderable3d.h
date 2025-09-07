#ifndef ECS_COMPONENT_RENDERABLE3D_H
#define ECS_COMPONENT_RENDERABLE3D_H

#include "core/util/types.h"
#include <stdbool.h>

struct renderable3d {
    bool visible;
    float opacity;
};

#endif // ECS_COMPONENT_RENDERABLE3D_H
