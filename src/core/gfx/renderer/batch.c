#include "core/gfx/renderer/batch.h"
#include "core/gfx/renderer/cmd.h"
#include "core/gfx/renderer/renderer.h"

static int renderer_batch_add_cmd(struct renderer_batch **out,
                                  struct array *batches,
                                  const struct renderer_cmd *cmd) {
    if (!out || !batches || !cmd)
        return CORE_NULLPTR;

    size_t batches_length = batches->length;
    for (size_t i = 0; i < batches_length; ++i) {
        struct renderer_batch *batch = NULL;
        int error = array_get((void **)&batch, batches, i);
        if (error)
            continue;

        if (strcmp(batch->mesh_id, cmd->mesh_id) == 0 &&
            strcmp(batch->shader_id, cmd->shader_id) == 0 &&
            strcmp(batch->texture_id, cmd->texture_id) == 0 &&
            batch->transparent == (cmd->transparent || cmd->opacity < 1.0f)) {
            *out = batch;
            return CORE_SUCCESS;
        }
    }

    struct renderer_batch new_batch = {0};
    int ret = snprintf(new_batch.mesh_id, GLOBALS_PATH_MAX, "%s", cmd->mesh_id);
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(new_batch.shader_id, GLOBALS_PATH_MAX, "%s", cmd->shader_id);
    if (ret < 0)
        return CORE_FAILURE;

    ret =
        snprintf(new_batch.texture_id, GLOBALS_PATH_MAX, "%s", cmd->texture_id);
    if (ret < 0)
        return CORE_FAILURE;

    new_batch.instanced = false;
    new_batch.transparent = (cmd->transparent || cmd->opacity < 1.0f);

    int error = array_push(batches, &new_batch);
    if (error)
        return error;

    struct renderer_batch *batch = NULL;
    error = array_get((void **)&batch, batches, batches_length);
    if (error)
        return error;

    error = array_init(&batch->cmds, RENDERER_START_CMD_CAPACITY,
                       sizeof(struct renderer_cmd));
    if (error)
        return error;

    *out = batch;
    return CORE_SUCCESS;
}

int renderer_batch_build(struct array *batches, struct array *cmds) {
    if (!batches || !cmds)
        return CORE_NULLPTR;

    for (size_t i = 0; i < cmds->length; ++i) {
        struct renderer_cmd *cmd = NULL;
        int error = array_get((void **)&cmd, cmds, i);
        if (error)
            continue;

        struct renderer_batch *batch = NULL;
        error = renderer_batch_add_cmd(&batch, batches, cmd);
        if (error)
            continue;

        error = array_push(&batch->cmds, cmd);
        if (error)
            continue;
    }

    for (size_t i = 0; i < batches->length; ++i) {
        struct renderer_batch *batch = NULL;
        int error = array_get((void **)&batch, batches, i);
        if (error)
            continue;

        batch->instanced = batch->cmds.length > 1;
    }

    return CORE_SUCCESS;
}

void renderer_batch_clear_all(struct renderer *in) {
    if (!in)
        return;

    for (size_t i = 0; i < in->batches.length; ++i) {
        struct renderer_batch *batch = NULL;
        if (array_get((void **)&batch, &in->batches, i) == CORE_SUCCESS &&
            batch)
            array_destroy(&batch->cmds);
    }

    array_clear(&in->batches);
}
