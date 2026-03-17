#include <cglm/ivec4.h>
#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/gfx/api.h>
#include <cls/gfx/batch.h>
#include <cls/gfx/cmd.h>
#include <cls/gfx/renderer.h>
#include <cls/util/allocator.h>
#include <cls/util/arena.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/profiler.h>

#define START_CMD_CAPACITY 128
#define BATCH_START_CAPACITY 64

struct renderer {
    struct allocator *alloc_frame;
    struct array *transparent_cmds;
    struct array *batches;
    struct table *batch_indices;
    struct gfx_api *api;
    ivec4 bg_color;
};

struct ecs_renderer_ctx {
    struct app *app;
    struct camera *active_cam;
    struct renderer *rend;
    struct renderable *ren;
    struct transform *cam_tf;
    struct transform *tf;
    struct ecs_world *world;
};

static int renderer_batch_add_cmd(struct renderer *rend,
                                  struct renderer_cmd *cmd) {
    if (!rend || !cmd || !cmd->ren)
        return CLS_NULLPTR;

    bool transparent = cmd->ren->transparent || cmd->ren->opacity < 1.0f;
    if (transparent)
        return array_push(&rend->transparent_cmds, (void *)&cmd);

    struct renderer_batch_data key = {
        .mesh_id = cmd->ren->mesh_id,
        .shader_id = cmd->ren->shader_id,
        .texture_id = cmd->ren->texture_id,
        .transparent = cmd->ren->transparent || cmd->ren->opacity < 1.0f,
    };

    void *found_ptr = NULL;
    int error = table_find(&found_ptr, rend->batch_indices, &key);
    if (error)
        return error;

    struct array *batch_cmds = NULL;
    size_t *found = found_ptr;
    size_t batch_index;
    if (!found) {
        struct renderer_batch new_batch = {.data = key};
        error = array_push(&rend->batches, &new_batch);
        if (error)
            return error;

        size_t batches_length = 0;
        error = array_length_get(&batches_length, rend->batches);
        if (error)
            return error;

        batch_index = batches_length - 1;
        error = table_insert(rend->batch_indices, &key, &batch_index);
        if (error)
            return error;

        void *batch_ptr = NULL;
        error = array_elem_get(&batch_ptr, rend->batches, batch_index);
        if (error)
            return error;

        struct renderer_batch *batch = batch_ptr;
        if (!batch)
            return error;

        error = array_create(&batch_cmds, BATCH_START_CAPACITY,
                             sizeof(struct renderer_ren *));
        if (error)
            goto cleanup;
    } else {
        batch_index = *found;
    }

    void *batch_ptr = NULL;
    error = array_elem_get(&batch_ptr, rend->batches, batch_index);
    if (error)
        return error;

    struct renderer_batch *batch = batch_ptr;
    if (!batch)
        return error;

    if (!found)
        batch->cmds = batch_cmds;

    return array_push(&batch->cmds, (void *)&cmd);

cleanup:
    array_destroy(batch_cmds);
    return error;
}

static int add_ecs_base_renderer_cmd(struct ecs_renderer_ctx *ctx) {
    arena_marker marker = 0;
    int error = arena_marker_save(&marker, ctx->app->arena_frame);
    if (error)
        return error;

    struct renderer_cmd *cmd = NULL;
    error = allocator_alloc((void **)&cmd, ctx->rend->alloc_frame,
                            sizeof(struct renderer_cmd),
                            alignof(struct renderer_cmd));
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
    arena_marker_restore(ctx->app->arena_frame, &marker);
    return error;
}

static int find_active_camera(struct camera **cam, struct transform **tf,
                              struct ecs_world *world) {
    if (!cam || !world)
        return CLS_NULLPTR;

    struct ecs_world_query *cam_query = NULL;
    int error = ecs_world_query_create(&cam_query, world, 3, "camera",
                                       "camera_active", "transform");
    if (error)
        return error;

    entity e = ENTITY_MAX;
    while (ecs_world_query_next(&e, cam_query) == CLS_SUCCESS &&
           e != ENTITY_MAX) {
        void *cam_ptr = NULL;
        error = ecs_world_query_component_get(&cam_ptr, cam_query, "camera", e);
        if (error)
            continue;

        void *tf_ptr = NULL;
        error =
            ecs_world_query_component_get(&tf_ptr, cam_query, "transform", e);
        if (error)
            continue;

        *cam = cam_ptr;
        *tf = tf_ptr;
    }

    ecs_world_query_destroy(cam_query);
    free(cam_query);
    return CLS_SUCCESS;
}

static int renderer_ecs_create_render_cmds(struct renderer *rend,
                                           struct app *app) {
    if (!app)
        return CLS_NULLPTR;

    struct table *ecs_worlds = NULL;
    int error = ecs_world_get_all(&ecs_worlds, app->ecs);
    if (error)
        return error;

    struct table_iterator *iter = NULL;
    error = table_iterator_create(&iter, ecs_worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *world_ptr = NULL;
        error = table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct ecs_world *world = world_ptr;
        if (!world)
            continue;

        struct camera *active_cam = NULL;
        struct transform *cam_tf = NULL;
        error = find_active_camera(&active_cam, &cam_tf, world);
        if (error)
            continue;

        if (!active_cam || !cam_tf)
            continue;

        ivec2 fb_size = {0};
        error = window_fb_size_get(fb_size, app->window);
        if (error)
            continue;

        camera_resize(active_cam, fb_size);
        if (active_cam->update) {
            error = camera_update(active_cam, cam_tf);
            if (error)
                LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                                 "Updating renderer active camera failed");
        }

        struct ecs_renderer_ctx ctx = {
            .app = app,
            .active_cam = active_cam,
            .cam_tf = cam_tf,
            .rend = rend,
            .world = world,
        };

        struct ecs_world_query *query = NULL;
        error =
            ecs_world_query_create(&query, world, 2, "renderable", "transform");
        if (error)
            continue;

        entity e = ENTITY_MAX;
        while (ecs_world_query_next(&e, query) == CLS_SUCCESS &&
               e != ENTITY_MAX) {

            void *ren_ptr = NULL;
            error =
                ecs_world_query_component_get(&ren_ptr, query, "renderable", e);
            if (error)
                continue;

            void *tf_ptr = NULL;
            error =
                ecs_world_query_component_get(&tf_ptr, query, "transform", e);
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

        ecs_world_query_destroy(query);
        free(query);
    }

    table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

static int renderer_frame_clear(struct renderer *rend, struct app *app) {
    if (!rend)
        return CLS_NULLPTR;

    size_t batches_length = 0;
    int error = array_length_get(&batches_length, rend->batches);
    if (error)
        return error;

    for (size_t i = 0; i < batches_length; ++i) {
        void *batch_ptr = NULL;
        error = array_elem_get(&batch_ptr, rend->batches, i);
        if (error)
            continue;

        struct renderer_batch *batch = batch_ptr;
        if (batch)
            array_destroy(batch->cmds);
    }

    error = array_clear(rend->transparent_cmds);
    if (error)
        return error;

    error = array_clear(rend->batches);
    if (error)
        return error;

    error = table_clear(rend->batch_indices);
    if (error)
        return error;

    error = arena_clear(app->arena_frame);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static void renderer_frame_destroy(struct renderer *rend) {
    if (!rend)
        return;

    size_t batches_length = 0;
    int error = array_length_get(&batches_length, rend->batches);
    if (error)
        return;

    for (size_t i = 0; i < batches_length; ++i) {
        void *batch_ptr = NULL;
        error = array_elem_get(&batch_ptr, rend->batches, i);
        if (error)
            continue;

        struct renderer_batch *batch = batch_ptr;
        if (batch)
            array_destroy(batch->cmds);
    }
}

static int cmd_depth_compare(const void *a, const void *b) {
    const struct renderer_cmd *ca = *(const struct renderer_cmd **)a;
    const struct renderer_cmd *cb = *(const struct renderer_cmd **)b;
    if (cb->depth > ca->depth)
        return 1;
    if (cb->depth < ca->depth)
        return -1;
    return 0;
}

static int renderer_sort_transparent(struct renderer *rend) {
    size_t count = 0;
    int error = array_length_get(&count, rend->transparent_cmds);
    if (error)
        return error;

    void *data = NULL;
    error = array_data_get(&data, rend->transparent_cmds);
    if (error)
        return error;

    qsort(data, count, sizeof(struct renderer_cmd *), cmd_depth_compare);
    return CLS_SUCCESS;
}

int renderer_create(struct renderer **rend, struct allocator *alloc_perm,
                    struct allocator *alloc_frame, struct gfx_api *api,
                    ivec4 bg_color) {
    if (!rend || !alloc_frame || !api)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error =
        allocator_alloc(&instance_ptr, alloc_perm, sizeof(struct renderer),
                        alignof(struct renderer));
    if (error)
        return error;

    struct renderer *instance = instance_ptr;
    if (!instance)
        return CLS_NULLPTR;

    instance->alloc_frame = alloc_frame;
    instance->api = api;
    glm_ivec4_copy(bg_color, instance->bg_color);

    error = array_create(&instance->transparent_cmds, START_CMD_CAPACITY,
                         sizeof(struct renderer_cmd *));
    if (error)
        goto cleanup;

    error = array_create(&instance->batches, START_CMD_CAPACITY,
                         sizeof(struct renderer_batch));
    if (error)
        goto cleanup;

    error = table_create(&instance->batch_indices, START_CMD_CAPACITY,
                         sizeof(struct renderer_batch_data), sizeof(size_t));
    if (error)
        goto cleanup;

    error = instance->api->init(instance->bg_color);
    if (error)
        goto cleanup;

    *rend = instance;
    return CLS_SUCCESS;

cleanup:
    renderer_destroy(instance);
    return error;
}

void renderer_destroy(struct renderer *rend) {
    if (!rend)
        return;

    renderer_frame_destroy(rend);
    array_destroy(rend->transparent_cmds);
    array_destroy(rend->batches);
    table_destroy(rend->batch_indices);
}

int renderer_swap_buffers(struct renderer *rend, GLFWwindow *window) {
    if (!rend)
        return CLS_NULLPTR;

    return rend->api->swap_buffers(window);
}

int renderer_on_resize(struct renderer *rend, int width, int height) {
    if (!rend)
        return CLS_NULLPTR;

    rend->api->on_resize(width, height);
    return CLS_SUCCESS;
}

int renderer_frame_create(struct renderer *rend, struct app *app) {
    if (!rend || !app)
        return CLS_NULLPTR;

    int error = renderer_frame_clear(rend, app);
    if (error)
        return error;

    error = renderer_ecs_create_render_cmds(rend, app);
    if (error)
        return error;

    error = renderer_sort_transparent(rend);
    if (error)
        return error;

    error = rend->api->draw_frame(app, rend->transparent_cmds, rend->batches);
    if (error)
        return error;

    return CLS_SUCCESS;
}
