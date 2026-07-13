#include <cglm/ivec4.h>
#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/batch.h>
#include <cls/gfx/gfx_api.h>
#include <cls/gfx/renderer.h>
#include <cls/util/arena.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>

static const size_t CMD_START_CAPACITY = 128;
static const size_t BATCH_START_CAPACITY = 128;

struct cls_renderer {
    struct cls_array *cmds;
    struct cls_array *batches;
    struct cls_array *transparent_batches;
    struct cls_table *batch_indices;
    struct cls_gfx_api *api;
    ivec4 bg_color;
};

struct cls_ecs_renderer_ctx {
    struct cls_app *app;
    struct camera *cam;
    struct cls_renderer *rend;
    struct renderable *ren;
    struct transform *cam_tf;
    struct transform *tf;
    struct cls_ecs_world *world;
    bool cam_dirty;
};

struct render_world_ctx {
    struct cls_ecs_world *world;
    struct camera *cam;
    struct transform *cam_tf;
};

static int add_cmd_to_render_batch(struct cls_renderer *rend,
                                   struct cls_renderer_cmd *cmd,
                                   size_t cmd_idx) {
    if (!rend || !cmd)
        return CLS_NULLPTR;

    struct cls_renderer_batch_data key = {
        .state = cmd->ren.state,
        .mesh_id = cmd->ren.mesh_id,
        .shader_id = cmd->ren.shader_id,
        .texture_id = cmd->ren.texture_id,
        .transparent = cmd->ren.opacity < 1.0f,
    };

    void *found_ptr = NULL;
    int error = cls_table_find(&found_ptr, rend->batch_indices, &key);
    if (error)
        return error;

    struct cls_array *new_batch_cmds = NULL;
    size_t *found = found_ptr;
    size_t batch_idx = 0;
    if (!found) {
        struct cls_renderer_batch new_batch = {.data = key, .cmds = NULL};
        error = cls_array_push(&rend->batches, &new_batch);
        if (error)
            return error;

        size_t batches_len = 0;
        error = cls_array_length_get(&batches_len, rend->batches);
        if (error)
            return error;

        batch_idx = batches_len - 1;
        error = cls_table_insert(rend->batch_indices, &key, &batch_idx);
        if (error)
            return error;

        error = cls_array_create(&new_batch_cmds, BATCH_START_CAPACITY,
                                 sizeof(size_t));
        if (error)
            goto cleanup;
    } else {
        batch_idx = *found;
    }

    void *batch_ptr = NULL;
    error = cls_array_elem_get(&batch_ptr, rend->batches, batch_idx);
    if (error)
        return error;

    struct cls_renderer_batch *batch = batch_ptr;
    if (!batch)
        return error;

    if (!found)
        batch->cmds = new_batch_cmds;

    size_t cmds_len = 0;
    error = cls_array_length_get(&cmds_len, batch->cmds);
    if (error)
        return error;

    batch->depth =
        (batch->depth * (float)cmds_len + cmd->depth) / ((float)cmds_len + 1);

    return cls_array_push(&batch->cmds, &cmd_idx);

cleanup:
    cls_array_destroy(new_batch_cmds);
    return error;
}

static int create_batches(struct cls_renderer *rend) {
    size_t cmds_len = 0;
    int error = cls_array_length_get(&cmds_len, rend->cmds);
    if (error)
        return error;

    for (size_t i = 0; i < cmds_len; ++i) {
        void *cmd_ptr = NULL;
        error = cls_array_elem_get(&cmd_ptr, rend->cmds, i);
        if (error)
            continue;

        struct cls_renderer_cmd *cmd = cmd_ptr;
        error = add_cmd_to_render_batch(rend, cmd, i);
        if (error)
            continue;
    }

    return CLS_SUCCESS;
}

static int renderer_draw(struct cls_renderer *rend, struct cls_app *app) {
    int error = rend->api->draw_batches(app, rend->cmds, rend->batches,
                                        &rend->transparent_batches);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static void renderer_frame_destroy(struct cls_renderer *rend) {
    if (!rend)
        return;

    size_t batches_len = 0;
    int error = cls_array_length_get(&batches_len, rend->batches);
    if (error)
        return;

    for (size_t i = 0; i < batches_len; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, rend->batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch = batch_ptr;
        if (batch)
            cls_array_destroy(batch->cmds);
    }
}

static int renderer_frame_reset(struct cls_renderer *rend,
                                struct cls_app *app) {
    if (!rend)
        return CLS_NULLPTR;

    size_t batches_length = 0;
    int error = cls_array_length_get(&batches_length, rend->batches);
    if (error)
        return error;

    for (size_t i = 0; i < batches_length; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, rend->batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch = batch_ptr;
        if (batch)
            cls_array_destroy(batch->cmds);
    }

    error = cls_array_clear(rend->cmds);
    if (error)
        return error;

    error = cls_array_clear(rend->batches);
    if (error)
        return error;

    error = cls_table_clear(rend->batch_indices);
    if (error)
        return error;

    error = cls_arena_clear(app->arena_frame);
    if (error)
        return error;

    return CLS_SUCCESS;
}

int cls_renderer_create(struct cls_renderer **rend, struct cls_mem *mem_perm,
                        struct cls_mem *mem_frame, struct cls_gfx_api *api,
                        const ivec4 bg_color) {
    if (!rend || !mem_perm || !mem_frame || !api)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error =
        cls_mem_alloc(&instance_ptr, mem_perm, sizeof(struct cls_renderer),
                      alignof(struct cls_renderer));
    if (error)
        return error;

    struct cls_renderer *instance = instance_ptr;

    instance->api = api;
    glm_ivec4_copy((int *)bg_color, instance->bg_color);

    error = cls_array_create(&instance->cmds, CMD_START_CAPACITY,
                             sizeof(struct cls_renderer_cmd));
    if (error)
        goto cleanup;

    error = cls_array_create(&instance->batches, CMD_START_CAPACITY,
                             sizeof(struct cls_renderer_batch));
    if (error)
        goto cleanup;

    error = cls_array_create(&instance->transparent_batches, CMD_START_CAPACITY,
                             sizeof(struct cls_renderer_batch *));
    if (error)
        goto cleanup;

    error = cls_table_create(&instance->batch_indices, CMD_START_CAPACITY,
                             sizeof(struct cls_renderer_batch_data),
                             sizeof(size_t));
    if (error)
        goto cleanup;

    error = instance->api->init(instance->bg_color);
    if (error)
        goto cleanup;

    *rend = instance;
    return CLS_SUCCESS;

cleanup:
    cls_renderer_destroy(instance);
    return error;
}

void cls_renderer_destroy(struct cls_renderer *rend) {
    if (!rend)
        return;

    renderer_frame_destroy(rend);
    cls_array_destroy(rend->cmds);
    cls_array_destroy(rend->batches);
    cls_array_destroy(rend->transparent_batches);
    cls_table_destroy(rend->batch_indices);
}

int cls_renderer_swap_buffers(struct cls_renderer *rend, GLFWwindow *window) {
    if (!rend || !window)
        return CLS_NULLPTR;

    return rend->api->swap_buffers(window);
}

int cls_renderer_on_resize(struct cls_renderer *rend, int width, int height) {
    if (!rend)
        return CLS_NULLPTR;

    rend->api->on_resize(width, height);
    return CLS_SUCCESS;
}

int cls_renderer_cmd_push(struct cls_renderer *rend, struct renderable *ren,
                          struct transform *tf, float depth) {
    if (!rend || !ren || !tf)
        return CLS_NULLPTR;

    struct cls_renderer_cmd cmd = {.ren = *ren, .tf = *tf, .depth = depth};
    return cls_array_push(&rend->cmds, &cmd);
}

int cls_renderer_frame_create(struct cls_renderer *rend, struct cls_app *app) {
    if (!rend || !app)
        return CLS_NULLPTR;

    rend->api->begin_frame();
    int error = create_batches(rend);
    if (error)
        return error;

    error = renderer_draw(rend, app);
    if (error)
        return error;

    return renderer_frame_reset(rend, app);
}
