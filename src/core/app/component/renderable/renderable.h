#ifndef ECS_COMPONENT_RENDERABLE_RENDERABLE_H
#define ECS_COMPONENT_RENDERABLE_RENDERABLE_H

#include "core/util/types.h"
#include <stdbool.h>

struct renderable {
    bool visible;
    float opacity;
};

#endif // ECS_COMPONENT_RENDERABLE_RENDERABLE_H
