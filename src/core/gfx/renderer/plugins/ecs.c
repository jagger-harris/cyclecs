#include "core/app/ecs.h"
#include "core/app/app.h"
#include "core/app/component/renderable/mesh.h"
#include "core/app/component/renderable/renderable.h"
#include "core/app/component/renderable/sprite.h"
#include "core/gfx/renderer/cmd.h"
#include "core/gfx/renderer/renderer.h"
#include "core/util/logger.h"
#include "core/util/util.h"

typedef int (*create_ecs_draw_cmd_fn)(struct renderer_cmd *out,
                                      void *component);

struct ecs_component_handler {
    const char *component_type_id;
    create_ecs_draw_cmd_fn create_draw_cmd;
};

static int create_ecs_mesh_draw_cmd(struct renderer_cmd *out, void *component) {
    struct mesh *mesh = component;

    int ret = snprintf(out->mesh_id, GLOBALS_PATH_MAX, "%s", mesh->mesh_id);
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(out->shader_id, GLOBALS_PATH_MAX, "%s", "mesh");
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(out->texture_id, GLOBALS_PATH_MAX, "%s", mesh->texture_id);
    if (ret < 0)
        return CORE_FAILURE;

    memcpy(&out->tint, &mesh->tint, sizeof(struct color));
    out->is_in_world = true;
    return CORE_SUCCESS;
}

static int create_ecs_sprite_draw_cmd(struct renderer_cmd *out,
                                      void *component) {
    struct sprite *sprite = component;

    int ret = snprintf(out->mesh_id, GLOBALS_PATH_MAX, "%s", "quad");
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(out->shader_id, GLOBALS_PATH_MAX, "%s", "sprite");
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(out->texture_id, GLOBALS_PATH_MAX, "%s", sprite->texture_id);
    if (ret < 0)
        return CORE_FAILURE;

    memcpy(&out->tint, &sprite->tint, sizeof(struct color));
    glm_vec2_copy(sprite->uv_offset, out->uv_offset);
    glm_vec2_copy(sprite->uv_scale, out->uv_scale);
    out->is_in_world = true;
    out->transparent = true;
    return CORE_SUCCESS;
}

static const struct ecs_component_handler COMPONENT_HANDLERS[] = {
    {"mesh", create_ecs_mesh_draw_cmd},
    {"sprite", create_ecs_sprite_draw_cmd},
};

#define COMPONENT_HANDLERS_LENGTH ARRAY_LENGTH(COMPONENT_HANDLERS)

static int process_entity(struct renderer *renderer, struct ecs_world *world,
                          u64 entity_id,
                          const struct ecs_component_handler *handler) {
    struct renderable *renderable = NULL;
    struct transform *transform = NULL;
    void *component = NULL;

    if (ecs_world_query_data((void **)&renderable, world, entity_id,
                             "renderable"),
        ecs_world_query_data((void **)&transform, world, entity_id,
                             "transform") ||
            ecs_world_query_data(&component, world, entity_id,
                                 handler->component_type_id)) {
        return CORE_FAILURE;
    }

    if (!renderable->visible)
        return CORE_SUCCESS;

    struct renderer_cmd cmd = {.opacity = renderable->opacity};
    memcpy(&cmd.transform, transform, sizeof(struct transform));

    int error = handler->create_draw_cmd(&cmd, component);
    if (error)
        return error;

    error = array_push(&renderer->cmds, &cmd);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Adding ecs draw cmd failed");
        return error;
    }

    return CORE_SUCCESS;
}

static int
process_world_for_component(struct renderer *renderer, struct ecs_world *world,
                            const struct ecs_component_handler *handler) {
    struct array *entities = NULL;
    int error = ecs_world_query_all_entities(&entities, world,
                                             handler->component_type_id);
    if (error)
        return error;

    for (size_t i = 0; i < entities->length; ++i) {
        u64 entity_id;
        array_get_cpy(&entity_id, entities, i);
        process_entity(renderer, world, entity_id, handler);
    }

    return CORE_SUCCESS;
}

static int ecs_plugin_create_draw_cmds(struct renderer *renderer,
                                       struct app *app) {
    if (!renderer || !app)
        return CORE_NULLPTR;

    struct table *ecs_worlds = NULL;
    int error = ecs_get_all_worlds(&ecs_worlds, &app->ecs);
    if (error)
        return error;

    struct table_iterator iter = {0};
    error = table_iterator_init(&iter, ecs_worlds);
    if (error)
        return error;

    while (table_iterator_next(&iter)) {
        struct ecs_world *world = iter.value;
        for (size_t i = 0; i < COMPONENT_HANDLERS_LENGTH; ++i)
            process_world_for_component(renderer, world,
                                        &COMPONENT_HANDLERS[i]);
    }

    return CORE_SUCCESS;
}

const struct renderer_plugin RENDERER_PLUGIN_ECS = {
    .name = "ecs", .create_cmds = ecs_plugin_create_draw_cmds};
