#ifndef ECS_COMPONENT_MESH2D_H
#define ECS_COMPONENT_MESH2D_H

#include "core/gfx/gl/mesh.h"
#include "core/util/globals.h"
#include "core/util/types.h"
#include <stdbool.h>

struct mesh2d {
    char texture_path[GLOBALS_STR_ID_MAX];
    struct gl_mesh mesh;
};

#endif // ECS_COMPONENT_MESH2D_H
