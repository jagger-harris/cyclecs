#include "base/game/game.h"
#include "base/ecs/sys/move_sys.h"
#include "base/game/constants.h"
#include "core/ecs/comp/transform_comp.h"
#include "core/ecs/sys/render_sys.h"
#include "core/util/logger.h"
#include <stdint.h>

static struct vertex quad_vertices[] = {
    {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
    {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
    {{0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
    {{-0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}}};
static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};
static size_t vertex_count = ARRAY_SIZE(quad_vertices);
static size_t index_count = ARRAY_SIZE(quad_indices);
static uint32_t rect;

err game_init(struct app *app) {
    struct assets *assets = &app->assets;
    struct ecs *ecs = &app->ecs;
    struct renderer *renderer = &app->window.renderer;
    err status = CORE_SUCCESS;

    assets_material_add(assets, "container", "quad", "container.jpg");
    assets_mesh_add(assets, "quad", quad_vertices, vertex_count, quad_indices,
                    index_count);

    struct ecs_handles handles = {0};

    status = ecs_comp_type_add(&handles.transformable, ecs,
                               sizeof(struct transform_comp));
    if (status)
        goto err;

    status =
        ecs_comp_type_add(&handles.drawable, ecs, sizeof(struct draw_call));
    if (status)
        goto err;

    status = ecs_system_add(&handles.render_sys, ecs, render_sys, renderer);
    if (status)
        goto err;

    status = ecs_system_add(&handles.move_sys, ecs, move_sys, renderer);
    if (status)
        goto err;

    status = ecs_entity_add(&rect, ecs);
    if (status)
        goto err;

    struct transform_comp rect_tf =
        (struct transform_comp){.pos = {0.0F, 0.0F, 1.0F},
                                .rot = {0.0F, 0.0F, 1.0F},
                                .scale = {1.0F, 1.0F, 1.0F},
                                .rot_deg = 45.0F};

    status = ecs_comp_add(ecs, rect, handles.transformable, &rect_tf);
    if (status)
        goto err;

    struct draw_call rect_draw = (struct draw_call){
        .mat = NULL,
        .mesh = NULL,
        .tf = rect_tf,
        .camera_dist = 0,
        .visible = true,
    };

    assets_material_get(&rect_draw.mat, assets, "container");
    assets_mesh_get(&rect_draw.mesh, assets, "quad");

    status = ecs_comp_add(ecs, rect, handles.drawable, &rect_draw);
    if (status)
        goto err;

    status = ecs_init_handles(ecs, &handles, sizeof(struct ecs_handles));
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init game failed");
    return status;
}

err game_run(struct app *app) {
    struct renderer *renderer = &app->window.renderer;
    struct input *input = &app->window.input;
    err status = CORE_SUCCESS;

    if (input->keys[GLFW_KEY_W])
        renderer_camera_move(renderer, 0.0F, 0.01F, 0.0F);

    if (input->keys[GLFW_KEY_S])
        renderer_camera_move(renderer, 0.0F, -0.01F, 0.0F);

    if (input->keys[GLFW_KEY_A])
        renderer_camera_move(renderer, 0.01F, 0.0F, 0.0F);

    if (input->keys[GLFW_KEY_D])
        renderer_camera_move(renderer, -0.01F, 0.0F, 0.0F);

    if (input->mouse_buttons[GLFW_MOUSE_BUTTON_LEFT]) {
        logger_log(LOGGER_DEBUG, "%f %f", input->cursor_pos[0],
                   input->cursor_pos[1]);
    }

    return status;
}
