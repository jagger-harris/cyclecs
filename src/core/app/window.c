#include "core/app/window.h"
#include "core/gfx/gl/renderer.h"
#include "core/gfx/renderer.h"
#include "core/util/logger.h"

static void error_callback(int status, const char *msg) {
    LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", msg);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
    struct window *glfw_user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    if (!glfw_user_window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_NULLPTR, "%s",
                         "Framebuffer resize failed");
        return;
    }

    glViewport(0, 0, width, height);
    renderer_resize(&glfw_user_window->renderer, width, height);
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

    struct window *glfw_user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    input_mouse_button(&glfw_user_window->input, button, action);
}

static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    struct window *glfw_user_window =
        (struct window *)glfwGetWindowUserPointer(window);

    glfw_user_window->input.cursor_pos[0] = (float)xpos;
    glfw_user_window->input.cursor_pos[1] = (float)ypos;
}

int window_init(struct window *in, int width, int height, const char *title,
                bool vsync) {
    if (!in || !title)
        return CORE_NULLPTR;

    glfwSetErrorCallback(error_callback);

    // TODO: Use for renderdoc as renderdoc and address sanitizer clash
#ifndef DEBUG
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

    if (!glfwInit()) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s", "Init GLFW failed");
        return CORE_GLFW;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    in->glfw_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!in->glfw_window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Creating GLFW window failed");
        glfwTerminate();
        in->glfw_window = NULL;
        return CORE_GLFW;
    }

    glfwMakeContextCurrent(in->glfw_window);
    glfwSetWindowUserPointer(in->glfw_window, in);
    glfwSetFramebufferSizeCallback(in->glfw_window, framebuffer_size_callback);
    glfwSetKeyCallback(in->glfw_window, key_callback);
    glfwSetMouseButtonCallback(in->glfw_window, mouse_button_callback);
    glfwSetCursorPosCallback(in->glfw_window, cursor_pos_callback);
    glfwSwapInterval(vsync);

    // TODO: Add support for multiple apis via an options file
    int status = CORE_SUCCESS;
    status = renderer_init(&in->renderer, (float)width / (float)height,
                           gl_renderer_init, gl_renderer_swap_buffers,
                           gl_renderer_on_resize, gl_renderer_draw_frame);
    if (status)
        return status;

    status = renderer_use(&in->renderer);
    if (status)
        return status;

    return CORE_SUCCESS;
}

void window_destroy(struct window *in) {
    if (!in)
        return;

    if (in->glfw_window)
        glfwDestroyWindow(in->glfw_window);

    glfwTerminate();
}
