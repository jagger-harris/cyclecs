#include "core/app/window.h"
#include "core/gfx/renderer/plugins/plugins.h"
#include "core/util/logger.h"

static void error_callback(int error, const char *msg) {
    LOGGER_LOG_ERROR(LOGGER_ERROR, error, "GLFW error: %s", msg);
}

static void framebuffer_size_callback(GLFWwindow *glfw_window, int width,
                                      int height) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Framebuffer resize failed");
        return;
    }

    if (width == 0 || height == 0)
        return;

    window->size[0] = width;
    window->size[1] = height;
    window->aspect_ratio = (height > 0) ? (float)width / (float)height : 1.0f;
    renderer_on_resize(&window->renderer, width, height);
}

static void key_callback(GLFWwindow *glfw_window, int key, int scancode,
                         int action, int mods) {
    (void)scancode;
    (void)mods;

    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s", "Getting key failed");
        return;
    }

    int error = input_key(&window->input, key, action);
    if (error)
        return;
}

static void mouse_button_callback(GLFWwindow *glfw_window, int button,
                                  int action, int mods) {
    (void)mods;

    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Getting mouse button failed");
        return;
    }

    int error = input_mouse_button(&window->input, button, action);
    if (error)
        return;
}

static void scroll_callback(GLFWwindow *glfw_window, double offset_x,
                            double offset_y) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Getting mouse button failed");
        return;
    }

    glm_vec2_copy((vec2){(float)offset_x, (float)offset_y},
                  window->input.scroll_offset);
}

static void cursor_pos_callback(GLFWwindow *glfw_window, double pos_x,
                                double pos_y) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Getting cursor pos failed");
        return;
    }

    glm_vec2_copy((vec2){(float)pos_x, (float)pos_y}, window->input.cursor_pos);
}

int window_init(struct window *out, struct gfx_api *api, ivec2 size,
                const char *title, bool vsync, struct color bg_color) {
    if (!out || !title)
        return CORE_NULLPTR;

    glfwSetErrorCallback(error_callback);

    int error = CORE_SUCCESS;
    if (!glfwInit()) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s", "Init GLFW failed");
        error = CORE_GLFW;
        goto cleanup;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    out->glfw_window = glfwCreateWindow(size[0], size[1], title, NULL, NULL);
    if (!out->glfw_window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_GLFW, "%s",
                         "Creating GLFW window failed");
        glfwTerminate();
        error = CORE_GLFW;
        goto cleanup;
    }

    glfwMakeContextCurrent(out->glfw_window);
    glfwSetWindowUserPointer(out->glfw_window, out);
    glfwSetCursorPosCallback(out->glfw_window, cursor_pos_callback);
    glfwSetFramebufferSizeCallback(out->glfw_window, framebuffer_size_callback);
    glfwSetKeyCallback(out->glfw_window, key_callback);
    glfwSetMouseButtonCallback(out->glfw_window, mouse_button_callback);
    glfwSetScrollCallback(out->glfw_window, scroll_callback);
    glfwSwapInterval(vsync);

    error = ui_init(&out->ui);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init window gui failed");
        goto cleanup;
    }

    error = input_init(&out->input);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init window input failed");
        goto cleanup;
    }

    error = renderer_init(&out->renderer, api, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Init window renderer failed");
        goto cleanup;
    }

    error = timing_init(&out->timing);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Init window timing failed");
        goto cleanup;
    }

    glfwGetFramebufferSize(out->glfw_window, &out->size[0], &out->size[1]);
    return CORE_SUCCESS;

cleanup:
    window_destroy(out);
    return error;
}

void window_destroy(struct window *in) {
    if (!in)
        return;

    if (in->glfw_window) {
        glfwDestroyWindow(in->glfw_window);
        in->glfw_window = NULL;
    }

    ui_destroy(&in->ui);
    renderer_destroy(&in->renderer);
    glfwTerminate();
}

int window_should_close(bool *out, struct window *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = glfwWindowShouldClose(in->glfw_window);
    return CORE_SUCCESS;
}

int window_size_get(ivec2 out, struct window *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    out[0] = in->size[0];
    out[1] = in->size[1];
    return CORE_SUCCESS;
}
