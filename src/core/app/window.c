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
    logger_log(LOGGER_ERROR, description, error_code);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    if (!user_window || !user_window->renderer) {
        logger_log(LOGGER_ERROR, "Failed to resize framebuffer",
                   CORE_INVALID_NULLPTR);
        return;
    }

    renderer_resize(user_window->renderer, width, height);
}

int window_new(window **out, arena *in, int width, int height,
               const char *title, int vsync) {
    int error = CORE_SUCCESS;

    glfwSetErrorCallback(error_callback);

    error = arena_alloc((void **)out, in, sizeof(window), _Alignof(window));
    if (error)
        goto error;

    if (!out || !title) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    if (!glfwInit()) {
        error = CORE_FAILURE;
        goto error;
    }

    (*out)->native = NULL;
    (*out)->native = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!(*out)->native) {
        arena_free_last(in);
        glfwTerminate();
        error = CORE_FAILURE;
        goto error;
    }

    glfwMakeContextCurrent((*out)->native);
    glfwSetWindowUserPointer((*out)->native, out);
    glfwSetFramebufferSizeCallback((*out)->native, framebuffer_size_callback);
    glfwSwapInterval(vsync);
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to make new window", error);
    return error;
}

int window_delete(window *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    glfwTerminate();

    if (in->native) {
        glfwDestroyWindow(in->native);
    }

    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to delete window", error);
    return error;
}

int window_get_native_window(GLFWwindow **out, window *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    if (!in->native) {
        error = CORE_FAILURE;
        goto error;
    }

    *out = in->native;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to get native window", error);
    return error;
}

int window_should_close(int *out, window *in) {
    int error = CORE_SUCCESS;

    if (!out || !in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    if (!in->native) {
        error = CORE_FAILURE;
        goto error;
    }

    *out = glfwWindowShouldClose(in->native);
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to get if window should close", error);
    return error;
}

int window_set_renderer(window *in, renderer *api) {
    int error = CORE_INVALID_NULLPTR;

    if (!in || !api) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    in->renderer = api;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to set window renderer api", error);
    return error;
}
