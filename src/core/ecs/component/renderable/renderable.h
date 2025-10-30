#ifndef ECS_COMPONENT_RENDERABLE_RENDERABLE_H
#define ECS_COMPONENT_RENDERABLE_RENDERABLE_H

#include "cglm/types.h"
#include "core/util/types.h"
#include <stdbool.h>

struct renderable {
    vec2 uv_offset;
    vec2 uv_scale;
    ivec4 tint;
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    float opacity;
    bool visible;
    bool transparent;
};

#endif // ECS_COMPONENT_RENDERABLE_RENDERABLE_H
