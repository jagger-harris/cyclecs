#ifndef GFX_RENDERER_CMD_H
#define GFX_RENDERER_CMD_H

#include "core/gfx/color.h"
#include "core/gfx/transform.h"
#include "core/util/globals.h"

struct renderer_cmd {
    char mesh_id[GLOBALS_PATH_MAX];
    char shader_id[GLOBALS_PATH_MAX];
    char texture_id[GLOBALS_PATH_MAX];
    struct transform transform;
    struct color tint;
    mat4 model;
    mat4 view;
    mat4 projection;
    vec2 uv_offset;
    vec2 uv_scale;
    float camera_distance;
    float opacity;
    bool transparent;
    bool instanced;
    bool is_in_world;
};

#endif // GFX_RENDERER_CMD_H
