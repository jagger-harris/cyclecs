#ifndef ECS_SYSTEM_MAIN_CAMERA_H
#define ECS_SYSTEM_MAIN_CAMERA_H

#include "core/app/app.h"
#include "core/app/window.h"
#include "core/ecs/component/camera.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>
#include <cglm/vec2.h>
#include <string.h>

static vec2 last_cursor = {0};
static bool dragging = false;

static float camera_speed = 1000.0f;
static vec2 camera_velocity = {0.0f, 0.0f};
static float velocity_decay = 0.86f;
static float velocity_threshold = 0.001f;

static float target_zoom = 0.0f;
static bool zoom_initialized = false;

static bool start = true;

static int wasd_movement(struct camera *active_camera, struct app *app,
                         float dt) {
    bool w_down = false;
    bool s_down = false;
    bool a_down = false;
    bool d_down = false;

    int error = window_input_key_down(&w_down, app->window, GLFW_KEY_W);
    if (error)
        return error;
    error = window_input_key_down(&s_down, app->window, GLFW_KEY_S);
    if (error)
        return error;
    error = window_input_key_down(&a_down, app->window, GLFW_KEY_A);
    if (error)
        return error;
    error = window_input_key_down(&d_down, app->window, GLFW_KEY_D);
    if (error)
        return error;

    if (w_down)
        camera_move(active_camera, (vec3){0.0f, -camera_speed * dt, 0.0f});
    if (s_down)
        camera_move(active_camera, (vec3){0.0f, camera_speed * dt, 0.0f});
    if (a_down)
        camera_move(active_camera, (vec3){-camera_speed * dt, 0.0f, 0.0f});
    if (d_down)
        camera_move(active_camera, (vec3){camera_speed * dt, 0.0f, 0.0f});

    return CORE_SUCCESS;
}

static int mouse_drag(struct camera *active_camera, struct app *app) {
    struct window *window = app->window;
    bool left_mouse_down = false;
    bool middle_mouse_down = false;

    int error = window_input_mouse_down(&left_mouse_down, app->window,
                                        GLFW_MOUSE_BUTTON_LEFT);
    if (error)
        return error;

    error = window_input_mouse_down(&middle_mouse_down, app->window,
                                    GLFW_MOUSE_BUTTON_MIDDLE);
    if (error)
        return error;

    if (left_mouse_down || middle_mouse_down) {
        vec2 cursor_pos = {0};
        error = window_input_cursor_pos_get(cursor_pos, app->window);
        if (error)
            return error;

        if (!dragging) {
            dragging = true;
            glm_vec2_copy(cursor_pos, last_cursor);
            camera_velocity[0] = 0.0f;
            camera_velocity[1] = 0.0f;
        } else {
            vec2 current_world = {0.0f};
            vec2 last_world = {0.0f};
            ivec2 fb_size = {0};

            error = window_fb_size_get(fb_size, window);
            if (error)
                return error;

            camera_screen_to_world(current_world, active_camera, cursor_pos,
                                   fb_size);
            camera_screen_to_world(last_world, active_camera, last_cursor,
                                   fb_size);

            float world_delta_x = last_world[0] - current_world[0];
            float world_delta_y = last_world[1] - current_world[1];

            camera_velocity[0] = world_delta_x * 60.0f;
            camera_velocity[1] = world_delta_y * 60.0f;
            camera_move(active_camera,
                        (vec3){world_delta_x, world_delta_y, 0.0f});
        }

        glm_vec2_copy(cursor_pos, last_cursor);
    } else {
        dragging = false;
    }

    return CORE_SUCCESS;
}

static int camera_momentum(struct camera *active_camera, float dt) {
    if (!dragging) {
        if (fabsf(camera_velocity[0]) > velocity_threshold ||
            fabsf(camera_velocity[1]) > velocity_threshold) {
            camera_move(active_camera, (vec3){camera_velocity[0] * dt,
                                              camera_velocity[1] * dt, 0.0f});

            float decay = powf(velocity_decay, dt * 60.0f);
            camera_velocity[0] *= decay;
            camera_velocity[1] *= decay;
        } else {
            camera_velocity[0] = 0.0f;
            camera_velocity[1] = 0.0f;
        }
    }

    return CORE_SUCCESS;
}

static int zoom(struct camera *active_camera, struct app *app, float dt) {
    vec2 scroll_offset = {0};
    int error = window_input_scroll_offset_get(scroll_offset, app->window);
    if (error)
        return error;

    if (!zoom_initialized) {
        target_zoom = active_camera->zoom;
        zoom_initialized = true;
    }

    if (fabsf(scroll_offset[1]) > 0.0001f) {
        float zoom_delta = scroll_offset[1] * 0.1f;
        target_zoom *= (1.0f + zoom_delta);
        if (target_zoom < 0.2f)
            target_zoom = 0.2f;
        if (target_zoom > 1.0f)
            target_zoom = 1.0f;
    }

    float current_zoom = active_camera->zoom;
    if (fabsf(current_zoom - target_zoom) > 0.001f) {
        vec2 cursor_pos = {0};
        error = window_input_cursor_pos_get(cursor_pos, app->window);
        if (error)
            return error;

        ivec2 fb_size = {0};
        error = window_fb_size_get(fb_size, app->window);
        if (error)
            return error;

        vec2 world_before = {0};
        camera_screen_to_world(world_before, active_camera, cursor_pos,
                               fb_size);

        float lerp_factor = 1.0f - powf(1.0f - 0.15f, dt * 60.0f);
        float new_zoom =
            current_zoom + (target_zoom - current_zoom) * lerp_factor;

        active_camera->zoom = new_zoom;

        vec2 world_after = {0};
        camera_screen_to_world(world_after, active_camera, cursor_pos, fb_size);

        float dx = world_before[0] - world_after[0];
        float dy = world_before[1] - world_after[1];
        camera_move(active_camera, (vec3){dx, dy, 0.0f});
    }

    return CORE_SUCCESS;
}

int main_camera_system(struct ecs_world_query *query, struct app *app) {
    if (!query || !query->world || !app)
        return CORE_NULLPTR;

    float dt = 0;
    int error = window_timing_dt_get(&dt, app->window);
    if (error)
        return error;

    struct camera *active_camera = NULL;
    u32 entity = U32_MAX;
    while (ecs_world_query_next(&entity, query) == CORE_SUCCESS &&
           entity != U32_MAX) {
        struct camera *camera = NULL;
        error = ecs_world_query_get((void **)&camera, query, ECS_COMP_CAMERA,
                                    entity);
        if (error || !camera)
            continue;

        if (camera->active) {
            active_camera = camera;
            break;
        }
    }

    if (!active_camera)
        return CORE_SUCCESS;

    error = wasd_movement(active_camera, app, dt);
    if (error)
        return error;

    error = mouse_drag(active_camera, app);
    if (error)
        return error;

    error = camera_momentum(active_camera, dt);
    if (error)
        return error;

    error = zoom(active_camera, app, dt);
    if (error)
        return error;

    ivec2 fb_size = {0};
    error = window_fb_size_get(fb_size, app->window);
    if (error)
        return error;

    if (start) {
        vec3 camera_start_pos = {-(float)fb_size[0] * 0.5f,
                                 -(float)fb_size[1] * 0.5f, 0.0f};
        camera_set_pos(active_camera, camera_start_pos);
        start = false;
    }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_MAIN_CAMERA_H
