#include <GLFW/glfw3.h>
#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/logger.h>
#include <stdio.h>

int ui_button_system(struct ecs_world_query *query, struct app *app,
                     void *user_data) {
    if (!query || !app)
        return CLS_NULLPTR;

    (void)user_data;

    struct camera *cam_data = NULL;
    entity cam = ENTITY_MAX;
    struct ecs_world *world = NULL;
    int error = ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    struct ecs_world_query *cam_query = NULL;
    error = ecs_world_query_create(&cam_query, world, 2, CLS_ECS_COMP_CAMERA,
                                   CLS_ECS_COMP_CAMERA_ACTIVE);
    if (error)
        return error;

    while (ecs_world_query_next(&cam, cam_query) == CLS_SUCCESS &&
           cam != ENTITY_MAX) {
        void *cam_ptr = NULL;
        error = ecs_world_query_component_get(&cam_ptr, cam_query,
                                              CLS_ECS_COMP_CAMERA, cam);
        if (error)
            continue;

        cam_data = cam_ptr;
        if (cam)
            break;
    }

    ecs_world_query_destroy(cam_query);

    if (!cam)
        return CLS_NULLPTR;

    u32 button = U32_MAX;
    while (ecs_world_query_next(&button, query) == CLS_SUCCESS &&
           button != U32_MAX) {
        void *button_comp_ptr = NULL;
        error = ecs_world_query_component_get(&button_comp_ptr, query,
                                              CLS_ECS_COMP_UI_BUTTON, button);
        if (error)
            continue;

        struct ui_button *button_comp = button_comp_ptr;
        if (!button)
            continue;

        void *tf_ptr = NULL;
        error = ecs_world_query_component_get(&tf_ptr, query,
                                              CLS_ECS_COMP_TRANSFORM, button);
        if (error)
            continue;

        struct transform *tf = tf_ptr;
        if (!button)
            continue;

        float half_width = tf->scale[0] * 0.5f;
        float half_height = tf->scale[1] * 0.5f;
        float min_x = tf->pos[0] - half_width;
        float max_x = tf->pos[0] + half_width;
        float min_y = tf->pos[1] - half_height;
        float max_y = tf->pos[1] + half_height;

        vec2 cursor_pos = {0};
        error = window_input_cursor_pos_get(cursor_pos, app->window);
        if (error)
            return error;

        ivec2 fb_size = {0};
        error = window_fb_size_get(fb_size, app->window);
        if (error)
            return error;

        vec2 cursor_world = {0};
        camera_screen_to_world(cursor_world, cam_data, cursor_pos, fb_size);

        bool inside = cursor_world[0] >= min_x && cursor_world[0] <= max_x &&
                      cursor_world[1] >= min_y && cursor_world[1] <= max_y;

        button_comp->hovering = inside;
        window_input_mouse_pressed(&button_comp->pressed, app->window,
                                   GLFW_MOUSE_BUTTON_LEFT);
        window_input_mouse_released(&button_comp->released, app->window,
                                    GLFW_MOUSE_BUTTON_LEFT);
        window_input_mouse_down(&button_comp->down, app->window,
                                GLFW_MOUSE_BUTTON_LEFT);
    }

    return CLS_SUCCESS;
}
