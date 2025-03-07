#include "app.h"
#include "../graphics/gl/gl_renderer.h"
#include "../graphics/renderer.h"
#include "../util/error.h"
#include "../util/logger.h"
#include "window.h"
#include <GLFW/glfw3.h>

struct app {
    window *window;
    renderer *renderer;
};

int app_new(app *out, int width, int height, const char *title) {
    int error = CORE_ERROR_SUCCESS;

    if (!out || !title) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    error = window_new(out->window, width, height, title);
    if (error)
        goto error;

    error = renderer_init(out->renderer, gl_renderer_swap_buffers,
                          gl_renderer_on_resize, gl_renderer_clear);
    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to make new app", error);
    return error;
}

int app_delete(app *in) {
    int error = CORE_ERROR_SUCCESS;

    if (!in) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    error = window_delete(in->window);
    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete app", error);
    return error;
}

int app_run(app *in) {
    int error = CORE_ERROR_SUCCESS;

    if (!in) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    GLFWwindow *current_window = NULL;

    int should_close = 0;
    while (!should_close) {
        error = window_should_close(&should_close, in->window);
        if (error)
            goto error;

        glfwPollEvents();

        if (current_window == NULL) {
            error = window_get_native_window(current_window, in->window);
            if (error)
                goto error;
        }

        renderer_clear(in->renderer);
        renderer_swap_buffers(in->renderer, current_window);
    }

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to run app", error);
    return CORE_ERROR_SUCCESS;
}
