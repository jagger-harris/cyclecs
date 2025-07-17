#ifndef GFX_DRAW_CALL_H
#define GFX_DRAW_CALL_H

#include "core/ecs/comp/transform_comp.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/material.h"

struct draw_call {
    struct gl_mesh *mesh;
    struct material *mat;
    struct transform_comp tf;
    float camera_dist;
    bool visible;
};

#endif // GFX_DRAW_CALL_H
