#ifndef CLS_RENDERER_BATCH_H
#define CLS_RENDERER_BATCH_H

#include <cls/ecs/component/components.h>
#include <cls/util/array.h>
#include <cls/util/types.h>
#include <stdbool.h>

struct cls_renderer_batch_data {
    struct render_state state;
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    bool transparent;
};

struct cls_renderer_batch {
    struct cls_renderer_batch_data data;
    struct cls_array *cmds;
    float depth;
};

#endif // CLS_RENDERER_BATCH_H
