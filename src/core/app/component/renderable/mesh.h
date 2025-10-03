#ifndef ECS_COMPONENT_RENDERABLE_MESH_H
#define ECS_COMPONENT_RENDERABLE_MESH_H

#include "core/gfx/color.h"
#include "core/util/globals.h"
#include "core/util/types.h"
#include <stdbool.h>

struct mesh {
    char texture_id[GLOBALS_PATH_MAX];
    char mesh_id[GLOBALS_PATH_MAX];
    struct color tint;
};

#endif // ECS_COMPONENT_RENDERABLE_MESH_H
