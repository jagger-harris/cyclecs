#ifndef ECS_SYSTEM_UI_BUTTON_H
#define ECS_SYSTEM_UI_BUTTON_H

#include "core/app/app.h"
#include "core/ecs/component/camera.h"
#include "core/ecs/component/renderable/ui.h"
#include "core/ecs/component/transform.h"
#include "core/ecs/ecs.h"
#include "core/ecs/world.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include <stdio.h>

int ui_button_system(struct ecs_world_query *query, struct app *app) {
    if (!query || !query->world || !app)
        return CORE_NULLPTR;

    struct camera *active_camera = NULL;
    u32 camera_entity = U32_MAX;
    struct ecs_world_query camera_query = {0};

    int error =
        ecs_world_query_init(&camera_query, query->world, 1, ECS_COMP_CAMERA);
    if (error)
        return error;

    while (ecs_world_query_next(&camera_entity, &camera_query) ==
               CORE_SUCCESS &&
           camera_entity != U32_MAX) {
        struct camera *camera = NULL;
        error = ecs_world_query_get((void **)&camera, &camera_query,
                                    ECS_COMP_CAMERA, camera_entity);
        if (error || !camera)
            continue;

        if (camera->active) {
            active_camera = camera;
            break;
        }
    }

    ecs_world_query_destroy(&camera_query);

    if (!active_camera)
        return CORE_SUCCESS;

    u32 entity = U32_MAX;
    while (ecs_world_query_next(&entity, query) == CORE_SUCCESS &&
           entity != U32_MAX) {
        struct ui_base *base = NULL;
        struct ui_button *button = NULL;
        struct transform *transform = NULL;

        error = ecs_world_query_get((void **)&base, query, ECS_COMP_UI_BASE,
                                    entity);
        if (error || !base)
            continue;

        error = ecs_world_query_get((void **)&button, query, ECS_COMP_UI_BUTTON,
                                    entity);
        if (error || !button)
            continue;

        error = ecs_world_query_get((void **)&transform, query,
                                    ECS_COMP_TRANSFORM, entity);
        if (error || !transform)
            continue;

        float half_width = transform->scale[0] * 0.5f;
        float half_height = transform->scale[1] * 0.5f;
        float min_x = transform->pos[0] - half_width;
        float max_x = transform->pos[0] + half_width;
        float min_y = transform->pos[1] - half_height;
        float max_y = transform->pos[1] + half_height;

        vec2 cursor_world = {0};
        camera_screen_to_world(cursor_world, active_camera,
                               app->window.input.cursor_pos, app->window.size);

        bool inside = cursor_world[0] >= min_x && cursor_world[0] <= max_x &&
                      cursor_world[1] >= min_y && cursor_world[1] <= max_y;

        button->hovering = inside;
        input_mouse_pressed(&button->pressed, &app->window.input,
                            GLFW_MOUSE_BUTTON_LEFT);
        input_mouse_released(&button->released, &app->window.input,
                             GLFW_MOUSE_BUTTON_LEFT);
        input_mouse_down(&button->down, &app->window.input,
                         GLFW_MOUSE_BUTTON_LEFT);
    }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_UI_BUTTON_H
