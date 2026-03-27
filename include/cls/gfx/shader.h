#ifndef CLS_SHADER_H
#define CLS_SHADER_H

#include <cls/gfx/gl/shader.h>
#include <cls/util/types.h>
#include <stdbool.h>

struct cls_shader {
    union {
        struct cls_gl_shader gl;
    };
};

struct cls_shader_info {
    const char *vert_src;
    const char *frag_src;
};

#endif // CLS_SHADER_H
