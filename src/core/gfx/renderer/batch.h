#ifndef GFX_RENDERER_BATCH_H
#define GFX_RENDERER_BATCH_H

#include "core/util/array.h"
#include "core/util/globals.h"
#include <stdbool.h>

struct renderer;

struct renderer_batch {
    char mesh_id[GLOBALS_PATH_MAX];
    char shader_id[GLOBALS_PATH_MAX];
    char texture_id[GLOBALS_PATH_MAX];
    struct array cmds;
    bool transparent;
    bool instanced;
};

int renderer_batch_build(struct array *batches, struct array *cmds);
void renderer_batch_clear_all(struct renderer *in);

#endif // GFX_RENDERER_BATCH_H
