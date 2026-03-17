#ifndef CLS_SHADER_H
#define CLS_SHADER_H

#include <cls/gfx/gl/shader.h>
#include <cls/util/types.h>
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

#endif // CLS_SHADER_H
