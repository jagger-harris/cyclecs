#include "core/app/app.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"

err app_init(struct app *in, int width, int height, const char *title) {
    err status = CORE_SUCCESS;

    if (!in || !title) {
        status = CORE_NULLPTR;
        goto err;
    }

    // TODO: Add checking vsync via an options file
    int vsync = 1;

    status = ecs_init(&in->ecs);
    if (status)
        goto err;

    status = assets_init(&in->assets);
    if (status)
        goto err;

    status = window_init(&in->window, width, height, title, vsync);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init app failed");
    return status;
}

void app_destroy(struct app *in) {
    if (!in)
        return;

    ecs_destroy(&in->ecs);
    assets_destroy(&in->assets);
    window_destroy(&in->window);
}

err app_run(struct app *in, const game_init_fn init,
            const game_update_fn update) {
    err status = CORE_SUCCESS;

    if (!in || !init || !update) {
        status = CORE_NULLPTR;
        goto err;
    }

    GLFWwindow *current_glfw_window = in->window.glfw_window;

    status = init(in);
    if (status)
        goto err;

    int should_close = 0;
    while (!should_close) {
        should_close = glfwWindowShouldClose(in->window.glfw_window);
        glfwPollEvents();

        status = update(in);
        if (status)
            goto err;

        status = ecs_update_all(&in->ecs);
        if (status)
            goto err;

        status = renderer_render_frame(&in->window.renderer, &in->assets);
        if (status)
            goto err;

        status =
            renderer_swap_buffers(&in->window.renderer, current_glfw_window);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Running app failed");
    return status;
}
