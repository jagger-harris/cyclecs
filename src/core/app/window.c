#include "core/app/window.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>

static void err_callback(err err, const char *msg) {
    logger_log_err(LOGGER_ERR, err, msg);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    if (!user_window || !user_window->renderer) {
        logger_log_err(LOGGER_ERR, CORE_NULLPTR, "Framebuffer resize failed");
        return;
    }

    glViewport(0, 0, width, height);
    user_window->renderer->camera.aspect_ratio = (float)width / (float)height;
    user_window->renderer->camera.update = true;

    renderer_resize(user_window->renderer, width, height);
}

err window_init(struct window *out, int width, int height, const char *title,
                int vsync) {
    err status = CORE_SUCCESS;

    glfwSetErrorCallback(err_callback);

    if (!out || !title) {
        status = CORE_NULLPTR;
        goto err;
    }

    // TODO: FORCE TO USE X11 BECAUSE OF RENDERDOC, REMOVE THIS WHEN NOT
    // DEBUGGING
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

    if (!glfwInit()) {
        status = CORE_GLFW;
        goto err;
    }

    out->native = NULL;
    out->native = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!out->native) {
        glfwTerminate();
        out->native = NULL;
        status = CORE_GLFW;
        goto err;
    }

    glfwMakeContextCurrent(out->native);
    glfwSetWindowUserPointer(out->native, out);
    glfwSetFramebufferSizeCallback(out->native, framebuffer_size_callback);
    glfwSwapInterval(vsync);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init window failed");
    return status;
}

void window_destroy(struct window *in) {
    if (!in)
        return;

    if (in->native)
        glfwDestroyWindow(in->native);

    glfwTerminate();
}
