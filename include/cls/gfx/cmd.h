#ifndef CLS_RENDERER_CMD_H
#define CLS_RENDERER_CMD_H

#include <cglm/types.h>

struct renderable;
struct transform;

struct cls_renderer_cmd {
    mat4 mvp;
    struct renderable *ren;
    struct transform *tf;
    float depth;
};

#endif // CLS_RENDERER_CMD_H
