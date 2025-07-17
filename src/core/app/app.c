#include "core/app/app.h"
#include "core/ecs/ecs.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"

err app_init(struct app *out, struct arena *mem, int width, int height,
             const char *title) {
    err status = CORE_SUCCESS;

    if (!out || !mem || !title) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = arena_alloc((void **)out, mem, sizeof(struct app),
                         _Alignof(struct app));
    if (status)
        goto err;

    // TODO: Add checking vsync via an options file
    int vsync = 1;

    status = ecs_init(&out->ecs);
    if (status)
        goto err;

    status = assets_init(&out->assets, mem);
    if (status)
        goto err;

    status = window_init(&out->window, width, height, title, vsync);
    if (status)
        goto err;

    // TODO: Add support for multiple apis via an options file
    status = renderer_init(&out->renderer, (float)width / (float)height,
                           gl_renderer_init, gl_renderer_swap_buffers,
                           gl_renderer_on_resize, gl_renderer_render_frame);
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

    GLFWwindow *current_native_window = in->window.native;
    in->window.renderer = &in->renderer;
    in->renderer.init();

    status = init(&in->assets, &in->ecs, &in->renderer);
    if (status)
        goto err;

    int should_close = 0;
    while (!should_close) {
        should_close = glfwWindowShouldClose(in->window.native);
        glfwPollEvents();

        status = update();
        if (status)
            goto err;

        status = ecs_update_all(&in->ecs);
        if (status)
            goto err;

        status = renderer_render_frame(&in->renderer, &in->assets);
        if (status)
            goto err;

        status = renderer_swap_buffers(&in->renderer, current_native_window);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Running app failed");
    return status;
}
