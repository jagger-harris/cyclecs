#ifndef GFX_RENDERER_BATCH_H
#define GFX_RENDERER_BATCH_H

#include "core/util/array.h"
#include "core/util/globals.h"
#include "core/util/types.h"
#include <stdbool.h>

struct renderer_batch_data {
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    bool transparent;
};

struct renderer_batch {
    struct renderer_batch_data data;
    struct array *cmds;
};

#endif // GFX_RENDERER_BATCH_H
