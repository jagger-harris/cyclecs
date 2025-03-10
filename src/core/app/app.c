#include "app.h"
#include "../graphics/gl/gl_renderer.h"
#include "../graphics/renderer.h"
#include "../util/error.h"
#include "../util/logger.h"
#include "window.h"

struct app {
    window *window;
    renderer *renderer;
};

int app_new(app **out, arena *in, int width, int height, const char *title) {
    int error = CORE_SUCCESS;

    if (!in || !title) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    error = arena_alloc((void **)out, in, sizeof(app), _Alignof(app));
    if (error)
        goto error;

    /* TODO: Eventually need to add checking vsync through an options file */
    int vsync = 1;

    error = window_new(&(*out)->window, in, width, height, title, vsync);
    if (error)
        goto error;

    error = renderer_init(&(*out)->renderer, in, gl_renderer_init,
                          gl_renderer_swap_buffers, gl_renderer_on_resize,
                          gl_renderer_clear);
    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to make new app", error);
    return error;
}

int app_delete(app *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    error = window_delete(in->window);
    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to delete app", error);
    return error;
}

int app_run(app *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    GLFWwindow *current_native_window = NULL;
    if (current_native_window == NULL) {
        error = window_get_native_window(&current_native_window, in->window);
        if (error)
            goto error;
    }

    renderer_use(in->renderer);

    int should_close = 0;
    while (!should_close) {
        error = window_should_close(&should_close, in->window);
        if (error)
            goto error;

        glfwPollEvents();

        renderer_clear(in->renderer);
        renderer_swap_buffers(in->renderer, current_native_window);
    }

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to run app", error);
    return error;
}
