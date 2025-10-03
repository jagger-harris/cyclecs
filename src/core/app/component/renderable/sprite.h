#ifndef ECS_COMPONENT_RENDERABLE_SPRITE_H
#define ECS_COMPONENT_RENDERABLE_SPRITE_H

#include "cglm/types.h"
#include "core/gfx/color.h"
#include "core/util/globals.h"

struct sprite {
    char texture_id[GLOBALS_PATH_MAX];
    struct color tint;
    vec2 uv_offset;
    vec2 uv_scale;
};

#endif // ECS_COMPONENT_RENDERABLE_SPRITE_H
