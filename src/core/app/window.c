#include "core/app/window.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>

struct window {
    GLFWwindow *native;
    renderer *renderer;
};

static void err_callback(err err, const char *msg) {
    logger_log(LOGGER_ERR, msg, err);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    if (!user_window || !user_window->renderer) {
        logger_log(LOGGER_ERR, "Failed to resize framebuffer",
                   CORE_INVALID_NULLPTR);
        return;
    }

    renderer_resize(user_window->renderer, width, height);
}

err window_new(window **out, arena *mem, int width, int height,
               const char *title, int vsync) {
    err err = CORE_SUCCESS;

    glfwSetErrorCallback(err_callback);

    if (!out || !mem || !title) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (!glfwInit()) {
        err = CORE_GLFW;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(window), _Alignof(window));
    if (err)
        goto err;

    (*out)->native = NULL;
    (*out)->native = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!(*out)->native) {
        arena_remove_last(mem);
        glfwTerminate();
        err = CORE_GLFW;
        goto err;
    }

    glfwMakeContextCurrent((*out)->native);
    glfwSetWindowUserPointer((*out)->native, *out);
    glfwSetFramebufferSizeCallback((*out)->native, framebuffer_size_callback);
    glfwSwapInterval(vsync);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new window", err);
    return err;
}

err window_delete(window *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    glfwTerminate();

    if (in->native)
        glfwDestroyWindow(in->native);

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete window", err);
    return err;
}

err window_get_native_window(GLFWwindow **out, window *in) {
    err err = CORE_SUCCESS;

    if (!in || !in->native) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->native;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get native window", err);
    return err;
}

err window_should_close(int *out, window *in) {
    err err = CORE_SUCCESS;

    if (!out || !in || !in->native) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = glfwWindowShouldClose(in->native);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get if window should close", err);
    return err;
}

err window_set_renderer(window *in, renderer *api) {
    err err = CORE_SUCCESS;

    if (!in || !api) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    in->renderer = api;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to set window renderer api", err);
    return err;
}
