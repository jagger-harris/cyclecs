#ifndef GFX_SHADER_H
#define GFX_SHADER_H

#include "core/gfx/gl/shader.h"
#include "core/util/types.h"
#include <stdbool.h>

struct shader {
    union {
        struct gl_shader gl;
    };
};

struct shader_info {
    const char *vert_src;
    const char *frag_src;
};

#endif // GFX_SHADER_H
