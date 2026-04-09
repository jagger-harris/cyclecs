#include <cglm/ivec4.h>
#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/batch.h>
#include <cls/gfx/cmd.h>
#include <cls/gfx/gfx_api.h>
#include <cls/gfx/renderer.h>
#include <cls/util/allocator.h>
#include <cls/util/arena.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/profiler.h>

#define START_CMD_CAPACITY 128
#define BATCH_START_CAPACITY 64

struct cls_renderer {
    struct cls_allocator *alloc_frame;
    struct cls_array *batches;
    struct cls_array *transparent_batches;
    struct cls_table *batch_indices;
    struct cls_gfx_api *api;
    ivec4 bg_color;
};

struct cls_ecs_renderer_ctx {
    struct cls_app *app;
    struct camera *active_cam;
    struct cls_renderer *rend;
    struct renderable *ren;
    struct transform *cam_tf;
    struct transform *tf;
    struct cls_ecs_world *world;
};

static int renderer_batch_add_cmd(struct cls_renderer *rend,
                                  struct cls_renderer_cmd *cmd) {
    if (!rend || !cmd || !cmd->ren)
        return CLS_NULLPTR;

    struct cls_renderer_batch_data key = {
        .state = cmd->ren->state,
        .mesh_id = cmd->ren->mesh_id,
        .shader_id = cmd->ren->shader_id,
        .texture_id = cmd->ren->texture_id,
        .transparent = cmd->ren->opacity < 1.0f,
    };

    void *found_ptr = NULL;
    int error = cls_table_find(&found_ptr, rend->batch_indices, &key);
    if (error)
        return error;

    struct cls_array *batch_cmds = NULL;
    size_t *found = found_ptr;
    size_t batch_index = 0;
    if (!found) {
        struct cls_renderer_batch new_batch = {.data = key, .cmds = NULL};
        error = cls_array_push(&rend->batches, &new_batch);
        if (error)
            return error;

        size_t batches_len = 0;
        error = cls_array_length_get(&batches_len, rend->batches);
        if (error)
            return error;

        batch_index = batches_len - 1;
        error = cls_table_insert(rend->batch_indices, &key, &batch_index);
        if (error)
            return error;

        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, rend->batches, batch_index);
        if (error)
            return error;

        struct cls_renderer_batch *batch = batch_ptr;
        if (!batch)
            return error;

        error = cls_array_create(&batch_cmds, BATCH_START_CAPACITY,
                                 sizeof(struct cls_renderer_cmd *));
        if (error)
            goto cleanup;
    } else {
        batch_index = *found;
    }

    void *batch_ptr = NULL;
    error = cls_array_elem_get(&batch_ptr, rend->batches, batch_index);
    if (error)
        return error;

    struct cls_renderer_batch *batch = batch_ptr;
    if (!batch)
        return error;

    if (!found)
        batch->cmds = batch_cmds;

    size_t cmds_length = 0;
    cls_array_length_get(&cmds_length, batch->cmds);
    batch->depth = (batch->depth * (float)cmds_length + cmd->depth) /
                   ((float)cmds_length + 1);

    return cls_array_push(&batch->cmds, (void *)&cmd);

cleanup:
    cls_array_destroy(batch_cmds);
    return error;
}

static int add_ecs_base_renderer_cmd(struct cls_ecs_renderer_ctx *ctx) {
    cls_arena_marker marker = 0;
    int error = cls_arena_marker_save(&marker, ctx->app->arena_frame);
    if (error)
        return error;

    struct cls_renderer_cmd *cmd = NULL;
    error = cls_allocator_alloc((void **)&cmd, ctx->rend->alloc_frame,
                                sizeof(struct cls_renderer_cmd),
                                alignof(struct cls_renderer_cmd));
    if (error)
        goto cleanup;

    if (!cmd)
        goto cleanup;

    cmd->ren = ctx->ren;

    vec3 delta = {0.0f};
    glm_vec3_sub(ctx->tf->pos, ctx->cam_tf->pos, delta);
    cmd->depth = glm_vec3_dot(ctx->active_cam->forward, delta);

    mat4 model = {{0.0f}};
    glm_mat4_identity(model);
    glm_translate(model, ctx->tf->pos);
    glm_rotate_at(model, ctx->tf->origin, ctx->tf->rot_angle,
                  ctx->tf->rot_axis);
    glm_scale(model, ctx->tf->scale);

    glm_mat4_identity(cmd->mvp);
    glm_mat4_mul(cmd->mvp, ctx->active_cam->projection, cmd->mvp);
    glm_mat4_mul(cmd->mvp, ctx->active_cam->view, cmd->mvp);
    glm_mat4_mul(cmd->mvp, model, cmd->mvp);

    error = renderer_batch_add_cmd(ctx->rend, cmd);
    if (error)
        goto cleanup;

    return CLS_SUCCESS;

cleanup:
    cls_arena_marker_restore(ctx->app->arena_frame, &marker);
    return error;
}

static int find_active_camera(struct camera **cam, struct transform **tf,
                              struct cls_ecs_world *world) {
    if (!cam || !world)
        return CLS_NULLPTR;

    struct cls_ecs_world_query *cam_query = NULL;
    int error =
        cls_ecs_world_query_create(&cam_query, world, 3, CLS_COMP_CAMERA,
                                   CLS_COMP_CAMERA_ACTIVE, CLS_COMP_TRANSFORM);
    if (error)
        return error;

    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, cam_query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        void *cam_ptr = NULL;
        error = cls_ecs_world_query_component_get(&cam_ptr, cam_query, e,
                                                  CLS_COMP_CAMERA);
        if (error)
            continue;

        void *tf_ptr = NULL;
        error = cls_ecs_world_query_component_get(&tf_ptr, cam_query, e,
                                                  CLS_COMP_TRANSFORM);
        if (error)
            continue;

        *cam = cam_ptr;
        *tf = tf_ptr;
    }

    cls_ecs_world_query_destroy(cam_query);
    return CLS_SUCCESS;
}

struct world_render_ctx {
    struct cls_ecs_world *world;
    struct camera *cam;
    struct transform *cam_tf;
};

static int world_render_ctx_compare(const void *a, const void *b) {
    const struct world_render_ctx *wa = a;
    const struct world_render_ctx *wb = b;

    if (wa->cam->layer > wb->cam->layer)
        return 1;

    if (wa->cam->layer < wb->cam->layer)
        return -1;

    return 0;
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

static int renderer_ecs_create_render_cmds(struct cls_renderer *rend,
                                           struct cls_app *app) {
    if (!app)
        return CLS_NULLPTR;

    struct cls_table *ecs_worlds = NULL;
    int error = cls_ecs_world_get_all(&ecs_worlds, app->ecs);
    if (error)
        return error;

    struct cls_array *world_ctxs = NULL;
    error = cls_array_create(&world_ctxs, 8, sizeof(struct world_render_ctx));
    if (error)
        return error;

    struct cls_table_iterator *iter = NULL;
    error = cls_table_iterator_create(&iter, ecs_worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *world_ptr = NULL;
        error = cls_table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world *world = world_ptr;
        if (!world)
            continue;

        struct camera *cam = NULL;
        struct transform *cam_tf = NULL;
        error = find_active_camera(&cam, &cam_tf, world);
        if (error || !cam || !cam_tf)
            continue;

        error = cls_array_push(&world_ctxs, &(struct world_render_ctx){
                                                .world = world,
                                                .cam = cam,
                                                .cam_tf = cam_tf,
                                            });
        if (error)
            continue;
    }

    cls_table_iterator_destroy(iter);

    size_t world_count = 0;
    error = cls_array_length_get(&world_count, world_ctxs);
    if (error)
        return error;

    void *world_ctxs_data = NULL;
    error = cls_array_data_get(&world_ctxs_data, world_ctxs);
    if (error)
        return error;

    qsort(world_ctxs_data, world_count, sizeof(struct world_render_ctx),
          world_render_ctx_compare);

    ivec2 fb_size = {0};
    error = cls_window_fb_size_get(fb_size, app->window);
    if (error)
        return error;

    for (size_t i = 0; i < world_count; ++i) {
        void *wctx_ptr = NULL;
        error = cls_array_elem_get(&wctx_ptr, world_ctxs, i);
        if (error || !wctx_ptr)
            continue;

        struct world_render_ctx *wctx = wctx_ptr;
        if (!wctx)
            continue;

        camera_resize(wctx->cam, fb_size);
        if (wctx->cam->update) {
            error = camera_update(wctx->cam, wctx->cam_tf);
            if (error)
                CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                     "Updating renderer active camera failed");
        }

        struct cls_ecs_renderer_ctx ctx = {
            .app = app,
            .active_cam = wctx->cam,
            .cam_tf = wctx->cam_tf,
            .rend = rend,
            .world = wctx->world,
        };

        struct cls_ecs_world_query *query = NULL;
        error = cls_ecs_world_query_create(
            &query, wctx->world, 2, CLS_COMP_RENDERABLE, CLS_COMP_TRANSFORM);
        if (error)
            continue;

        cls_entity e = CLS_ENTITY_MAX;
        while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS &&
               e != CLS_ENTITY_MAX) {
            void *ren_ptr = NULL;
            error = cls_ecs_world_query_component_get(&ren_ptr, query, e,
                                                      CLS_COMP_RENDERABLE);
            if (error)
                continue;

            void *tf_ptr = NULL;
            error = cls_ecs_world_query_component_get(&tf_ptr, query, e,
                                                      CLS_COMP_TRANSFORM);
            if (error)
                continue;

            struct renderable *ren = ren_ptr;
            struct transform *tf = tf_ptr;

            if (!ren || !tf || !ren->visible)
                continue;

            ctx.ren = ren;
            ctx.tf = tf;

            error = add_ecs_base_renderer_cmd(&ctx);
            if (error)
                continue;
        }

        cls_ecs_world_query_destroy(query);

        error = rend->api->draw_frame(app, rend->batches);
        if (error)
            continue;

        error = renderer_frame_reset(rend, app);
        if (error)
            return error;
    }

    cls_array_destroy(world_ctxs);
    return CLS_SUCCESS;
}

static void renderer_frame_destroy(struct cls_renderer *rend) {
    if (!rend)
        return;

    size_t batches_length = 0;
    int error = cls_array_length_get(&batches_length, rend->batches);
    if (error)
        return;

    for (size_t i = 0; i < batches_length; ++i) {
        void *batch_ptr = NULL;
        error = cls_array_elem_get(&batch_ptr, rend->batches, i);
        if (error)
            continue;

        struct cls_renderer_batch *batch = batch_ptr;
        if (batch)
            cls_array_destroy(batch->cmds);
    }
}

int cls_renderer_create(struct cls_renderer **rend,
                        struct cls_allocator *alloc_perm,
                        struct cls_allocator *alloc_frame,
                        struct cls_gfx_api *api, ivec4 bg_color) {
    if (!rend || !alloc_frame || !api)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_allocator_alloc(&instance_ptr, alloc_perm,
                                    sizeof(struct cls_renderer),
                                    alignof(struct cls_renderer));
    if (error)
        return error;

    struct cls_renderer *instance = instance_ptr;
    if (!instance)
        return CLS_NULLPTR;

    instance->alloc_frame = alloc_frame;
    instance->api = api;
    glm_ivec4_copy(bg_color, instance->bg_color);

    error = cls_array_create(&instance->batches, START_CMD_CAPACITY,
                             sizeof(struct cls_renderer_batch));
    if (error)
        goto cleanup;

    error = cls_array_create(&instance->transparent_batches, START_CMD_CAPACITY,
                             sizeof(struct cls_renderer_batch));
    if (error)
        goto cleanup;

    error = cls_table_create(&instance->batch_indices, START_CMD_CAPACITY,
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
    cls_array_destroy(rend->batches);
    cls_table_destroy(rend->batch_indices);
}

int cls_renderer_swap_buffers(struct cls_renderer *rend, GLFWwindow *window) {
    if (!rend)
        return CLS_NULLPTR;

    return rend->api->swap_buffers(window);
}

int cls_renderer_on_resize(struct cls_renderer *rend, int width, int height) {
    if (!rend)
        return CLS_NULLPTR;

    rend->api->on_resize(width, height);
    return CLS_SUCCESS;
}

int cls_renderer_frame_create(struct cls_renderer *rend, struct cls_app *app) {
    if (!rend || !app)
        return CLS_NULLPTR;

    rend->api->begin_frame();
    int error = renderer_ecs_create_render_cmds(rend, app);
    if (error)
        return error;

    return CLS_SUCCESS;
}
