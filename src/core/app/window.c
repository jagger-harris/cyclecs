#include "core/app/window.h"
#include "core/gfx/gl/renderer.h"
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

    if (!user_window) {
        logger_log_err(LOGGER_ERR, CORE_NULLPTR, "Framebuffer resize failed");
        return;
    }

    glViewport(0, 0, width, height);
    user_window->renderer.camera.aspect_ratio = (float)width / (float)height;
    user_window->renderer.camera.update = true;

    renderer_resize(&user_window->renderer, width, height);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
    // Prevent unused warning
    (void)scancode;
    (void)mods;

    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    input_key(&user_window->input, key, action);
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
    // Prevent unused warning
    (void)mods;

    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    input_mouse_button(&user_window->input, button, action);
}

static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    struct window *user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    user_window->input.cursor_pos[0] = (float)xpos;
    user_window->input.cursor_pos[1] = (float)ypos;
}

err window_init(struct window *in, int width, int height, const char *title,
                int vsync) {
    err status = CORE_SUCCESS;

    glfwSetErrorCallback(err_callback);

    if (!in || !title) {
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    in->glfw_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!in->glfw_window) {
        glfwTerminate();
        in->glfw_window = NULL;
        status = CORE_GLFW;
        goto err;
    }

    glfwMakeContextCurrent(in->glfw_window);
    glfwSetWindowUserPointer(in->glfw_window, in);
    glfwSetFramebufferSizeCallback(in->glfw_window, framebuffer_size_callback);
    glfwSetKeyCallback(in->glfw_window, key_callback);
    glfwSetMouseButtonCallback(in->glfw_window, mouse_button_callback);
    glfwSetCursorPosCallback(in->glfw_window, cursor_pos_callback);
    glfwSwapInterval(vsync);

    // TODO: Add support for multiple apis via an options file
    status = renderer_init(&in->renderer, (float)width / (float)height,
                           gl_renderer_init, gl_renderer_swap_buffers,
                           gl_renderer_on_resize, gl_renderer_render_frame);
    if (status)
        goto err;

    status = renderer_use(&in->renderer);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init window failed");
    return status;
}

void window_destroy(struct window *in) {
    if (!in)
        return;

    if (in->glfw_window)
        glfwDestroyWindow(in->glfw_window);

    glfwTerminate();
}
