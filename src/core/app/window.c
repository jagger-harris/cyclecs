#include "window.h"
#include "../graphics/renderer.h"
#include "../util/error.h"
#include "../util/logger.h"
#include <GLFW/glfw3.h>

struct window {
    GLFWwindow *native;
    renderer *renderer;
};

static void error_callback(int error_code, const char *description) {
    logger_log(LOGGER_LOG_LEVEL_ERROR, description, error_code);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    if (!user_window || !user_window->renderer) {
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to resizeframebuffer",
                   CORE_ERROR_INVALID_NULLPTR);
        return;
    }

    renderer_resize(user_window->renderer, width, height);
}

int window_new(window *out, int width, int height, const char *title) {
    int error = CORE_ERROR_SUCCESS;

    glfwSetErrorCallback(error_callback);

    if (!out || !title) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    if (!glfwInit()) {
        error = CORE_ERROR_FAILURE;
        goto error;
    }

    out->native = NULL;
    out->native = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!out->native) {
        glfwTerminate();
        error = CORE_ERROR_FAILURE;
        goto error;
    }

    glfwSetWindowUserPointer(out->native, out);
    glfwSetFramebufferSizeCallback(out->native, framebuffer_size_callback);

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to make new window", error);
    return CORE_ERROR_SUCCESS;
}

int window_delete(window *in) {
    int error = CORE_ERROR_SUCCESS;

    if (!in) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    glfwTerminate();

    if (in->native) {
        glfwDestroyWindow(in->native);
    }

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete window", error);
    return error;
}

int window_get_native_window(GLFWwindow *out, window *in) {
    int error = CORE_ERROR_SUCCESS;

    if (!out || !in) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    if (!in->native) {
        error = CORE_ERROR_FAILURE;
        goto error;
    }

    out = in->native;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to get native window", error);
    return CORE_ERROR_SUCCESS;
}

int window_should_close(int *out, window *in) {
    int error = CORE_ERROR_SUCCESS;

    if (!out || !in) {
        error = CORE_ERROR_INVALID_NULLPTR;
        goto error;
    }

    if (!in->native) {
        error = CORE_ERROR_FAILURE;
        goto error;
    }

    *out = glfwWindowShouldClose(in->native);

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to get if window should close",
               error);
    return CORE_ERROR_SUCCESS;
}

int window_set_renderer(window *in, renderer *api) {
    int error = CORE_ERROR_INVALID_NULLPTR;

    if (!in || !api) {
        error = CORE_ERROR_INVALID_NULLPTR;
        logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to set window renderer api",
                   error);

        return error;
    }

    in->renderer = api;
    return error;
}
