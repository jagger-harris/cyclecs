#include "core/gfx/renderer.h"
#include "core/app/app.h"
#include "core/ecs/component/camera.h"
#include "core/ecs/component/renderable/renderable.h"
#include "core/ecs/component/transform.h"
#include "core/ecs/world.h"
#include "core/gfx/batch.h"
#include "core/gfx/cmd.h"
#include "core/util/arena.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"

#define RENDERER_BATCH_START_CAPACITY 64

struct ecs_renderer_ctx {
    struct app *app;
    struct camera *active_camera;
    struct renderer *renderer;
    struct renderable *ren;
    struct transform *tf;
    struct ecs_world *world;
};

static int renderer_batch_add_cmd(struct renderer *in,
                                  struct renderer_cmd *cmd) {
    if (!in || !cmd || !cmd->ren)
        return CORE_NULLPTR;

    struct renderer_batch_data key = {
        .mesh_id = cmd->ren->mesh_id,
        .shader_id = cmd->ren->shader_id,
        .texture_id = cmd->ren->texture_id,
        .transparent = cmd->ren->transparent || cmd->ren->opacity < 1.0f,
    };

    size_t *found = NULL;
    int error = table_find((void **)&found, &in->batch_indices, &key);
    if (error)
        return error;

    size_t batch_index;
    if (!found) {
        struct renderer_batch new_batch = {.data = key};
        error = array_push(&in->batches, &new_batch);
        if (error)
            return error;

        batch_index = in->batches.length - 1;

        error = table_insert(&in->batch_indices, &key, &batch_index);
        if (error)
            return error;

        struct renderer_batch *batch = NULL;
        error = array_get((void **)&batch, &in->batches, batch_index);
        if (error)
            return error;

        error = array_init(&batch->cmds, RENDERER_BATCH_START_CAPACITY,
                           sizeof(struct renderer_ren *));
        if (error)
            return error;
    } else {
        batch_index = *found;
    }

    struct renderer_batch *batch = NULL;
    error = array_get((void **)&batch, &in->batches, batch_index);
    if (error)
        return error;

    return array_push(&batch->cmds, (void *)&cmd);
}

static int add_ecs_base_renderer_cmd(struct ecs_renderer_ctx *ctx) {
    struct renderer_cmd *cmd = NULL;
    int error =
        arena_alloc((void **)&cmd, &ctx->renderer->arena,
                    sizeof(struct renderer_cmd), alignof(struct renderer_cmd));
    if (error)
        goto cleanup;

    if (!cmd)
        goto cleanup;

    *cmd = (struct renderer_cmd){.ren = ctx->ren};

    mat4 model;
    glm_mat4_identity(model);
    glm_translate(model, ctx->tf->pos);
    glm_rotate_at(model, ctx->tf->origin, ctx->tf->rot_angle,
                  ctx->tf->rot_axis);
    glm_scale(model, ctx->tf->scale);

    glm_mat4_identity(cmd->mvp);
    glm_mat4_mul(cmd->mvp, ctx->active_camera->projection, cmd->mvp);
    glm_mat4_mul(cmd->mvp, ctx->active_camera->view, cmd->mvp);
    glm_mat4_mul(cmd->mvp, model, cmd->mvp);

    error = renderer_batch_add_cmd(ctx->renderer, cmd);
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    if (cmd)
        arena_remove_last(&ctx->renderer->arena);

    return error;
}

static int renderer_ecs_create_render_cmds(struct app *app) {
    if (!app)
        return CORE_NULLPTR;

    struct ecs_renderer_ctx ctx = {
        .app = app,
        .active_camera = NULL,
        .renderer = &app->window.renderer,
        .world = NULL,
    };

    struct table *ecs_worlds = NULL;
    int error = ecs_get_all_worlds(&ecs_worlds, &app->ecs);
    if (error)
        return error;

    struct table_iterator iter = {0};
    error = table_iterator_init(&iter, ecs_worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, &iter) == CORE_SUCCESS &&
           iter_next) {
        struct ecs_world *world = iter.value;
        struct array *camera_entities = NULL;
        error = ecs_world_query_all_entities(&camera_entities, world,
                                             ECS_COMP_CAMERA);
        if (error || !camera_entities)
            continue;

        struct camera *active_camera = NULL;
        for (size_t i = 0; i < camera_entities->length; ++i) {
            u32 entity_current = U32_MAX;
            array_get_cpy(&entity_current, camera_entities, i);

            error = ecs_world_query_data((void **)&active_camera, world,
                                         entity_current, ECS_COMP_CAMERA);
            if (error)
                continue;

            if (active_camera->active)
                break;
        }

        if (!active_camera)
            continue;

        camera_resize(active_camera, app->window.size);

        if (active_camera->update) {
            error = camera_update(active_camera);
            if (error)
                LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                                 "Updating renderer active camera failed");
        }

        ctx.active_camera = active_camera;
        ctx.world = world;

        struct ecs_world_query query = {0};
        error = ecs_world_query_init(&query, world, 2, ECS_COMP_RENDERABLE,
                                     ECS_COMP_TRANSFORM);
        if (error)
            continue;

        u32 entity = UINT32_MAX;
        while (ecs_world_query_next(&entity, &query) == CORE_SUCCESS &&
               entity != UINT32_MAX) {
            struct renderable *ren = NULL;
            struct transform *tf = NULL;

            error = ecs_world_query_get((void **)&ren, &query,
                                        ECS_COMP_RENDERABLE, entity);
            if (error)
                continue;

            error = ecs_world_query_get((void **)&tf, &query,
                                        ECS_COMP_TRANSFORM, entity);
            if (error)
                continue;

            if (!ren || !tf || !ren->visible)
                continue;

            ctx.ren = ren;
            ctx.tf = tf;

            error = add_ecs_base_renderer_cmd(&ctx);
            if (error)
                continue;
        }
    }

    return CORE_SUCCESS;
}

int renderer_init(struct renderer *out, struct gfx_api *api, ivec4 bg_color) {
    if (!out)
        return CORE_NULLPTR;

    *out = (struct renderer){.api = api};
    glm_ivec4_copy(bg_color, out->bg_color);

    int error = arena_init(&out->arena, 1024L * 1024L * 50L);
    if (error)
        goto cleanup;

    error = array_init(&out->batches, RENDERER_START_CMD_CAPACITY,
                       sizeof(struct renderer_batch));
    if (error)
        goto cleanup;

    error = table_init(&out->batch_indices, RENDERER_START_CMD_CAPACITY,
                       sizeof(struct renderer_batch_data), sizeof(size_t));
    if (error)
        goto cleanup;

    error = out->api->init(out->bg_color);
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    renderer_destroy(out);
    return error;
}

void renderer_destroy(struct renderer *in) {
    if (!in)
        return;

    arena_destroy(&in->arena);
    array_destroy(&in->batches);
    table_destroy(&in->batch_indices);
    table_destroy(&in->cameras);
}

int renderer_swap_buffers(struct renderer *in, GLFWwindow *window) {
    if (!in)
        return CORE_NULLPTR;

    return in->api->swap_buffers(window);
}

int renderer_on_resize(struct renderer *in, int width, int height) {
    if (!in)
        return CORE_NULLPTR;

    in->api->on_resize(width, height);
    return CORE_SUCCESS;
}

int renderer_draw_frame(struct renderer *in, struct app *app) {
    if (!in || !app)
        return CORE_NULLPTR;

    for (size_t i = 0; i < in->batches.length; ++i) {
        struct renderer_batch *batch = NULL;
        int error = array_get((void **)&batch, &in->batches, i);
        if (error)
            continue;

        if (batch)
            array_destroy(&batch->cmds);
    }

    array_clear(&in->batches);
    table_clear(&in->batch_indices);
    arena_clear(&in->arena);

    PROFILER_START(renderer_ecs_create_render_cmds);
    int error = renderer_ecs_create_render_cmds(app);
    if (error)
        return error;
    PROFILER_END(renderer_ecs_create_render_cmds);

    PROFILER_START(gl_draw_frame);
    error = in->api->draw_frame(app, &in->batches);
    if (error)
        return error;
    PROFILER_END(gl_draw_frame);

    return CORE_SUCCESS;
}
