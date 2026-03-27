#ifndef CLS_RENDERER_BATCH_H
#define CLS_RENDERER_BATCH_H

#include <cls/util/array.h>
#include <cls/util/globals.h>
#include <cls/util/types.h>
#include <stdbool.h>

struct cls_renderer_batch_data {
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    bool transparent;
};

struct cls_renderer_batch {
    struct cls_renderer_batch_data data;
    struct cls_array *cmds;
};

#endif // CLS_RENDERER_BATCH_H
