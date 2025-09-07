#include "core/app/app.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/error.h"
#include "core/util/logger.h"

#define ARENA_MEM_SIZE 1024 * 1024

int app_init(struct app *in, int width, int height, const char *title) {
    if (!in)
        return CORE_NULLPTR;

    if (!title)
        return CORE_INVALID_ARGS;

    int status = arena_init(&in->arena, ARENA_MEM_SIZE);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app arena failed");
        return status;
    }

    status = ecs_init(&in->ecs);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app ecs failed");
        return status;
    }

    status = assets_init(&in->assets);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app assets failed");
        return status;
    }

    status = app_time_init(&in->time);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app time failed");
        return status;
    }

    // TODO: Add checking vsync via an options file
    bool vsync = true;
    status = window_init(&in->window, width, height, title, vsync);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app window failed");
        return status;
    }

    return CORE_SUCCESS;
}

void app_destroy(struct app *in) {
    if (!in)
        return;

    arena_destroy(&in->arena);
    ecs_destroy(&in->ecs);
    assets_destroy(&in->assets);
    window_destroy(&in->window);
}

int app_run(struct app *in, const game_update_fn game_update) {
    if (!in || !game_update)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    while (!glfwWindowShouldClose(in->window.glfw_window)) {
        glfwPollEvents();

        status = game_update(in);
        if (status)
            LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                             "Updating game failed");

        status = ecs_update_all_worlds(&in->ecs);
        if (status)
            LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                             "Updating all ecs worlds failed");

        struct table_iterator iter;
        if (table_iterator_init(&iter, &in->ecs.worlds) == CORE_SUCCESS) {
            while (table_iterator_next(&iter)) {
                int status = renderer_draw_frame(&in->window.renderer,
                                                 &in->assets, iter.value);
                if (status) {
                    LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                                     "Drawing frame from ecs world failed");
                }
            }
        }

        status =
            renderer_swap_buffers(&in->window.renderer, in->window.glfw_window);
        if (status)
            LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                             "Swapping buffers failed");

        status = app_time_update(&in->time);
        if (status)
            LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                             "Updating app time failed");

        int fps = round(in->time.fps_avg);
        LOGGER_LOG(LOGGER_DEBUG, "FPS: %i", fps);
    }

    return CORE_SUCCESS;
}
