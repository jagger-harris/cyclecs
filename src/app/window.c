#include <GLFW/glfw3.h>
#include <cglm/ivec2.h>
#include <cglm/vec2.h>
#include <cls/app/window.h>
#include <cls/gfx/renderer.h>
#include <cls/util/allocator.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <string.h>

struct input {
    bool keys[WINDOW_INPUT_KEYS_SIZE];
    bool last_keys[WINDOW_INPUT_KEYS_SIZE];
    bool mouse_buttons[WINDOW_INPUT_MOUSE_BUTTONS_SIZE];
    bool last_mouse_buttons[WINDOW_INPUT_MOUSE_BUTTONS_SIZE];
    vec2 cursor_pos;
    vec2 scroll_offset;
};

struct timing {
    float delta_time;
    float total;
    double last_frame;
    int frame_count;
    float fps;
    float fps_avg;
    float fps_timer;
};

struct window {
    struct input *input;
    struct renderer *renderer;
    struct timing *timing;
    ivec2 win_size;
    ivec2 fb_size;
    GLFWwindow *glfw_window;
};

static void error_callback(int error, const char *msg) {
    LOGGER_LOG_ERROR(LOGGER_ERROR, error, "GLFW error: %s", msg);
}

static void window_size_callback(GLFWwindow *glfw_window, int width,
                                 int height) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW framebuffer resize failed");
        return;
    }

    glm_ivec2_copy((ivec2){width, height}, window->win_size);
}

static void framebuffer_size_callback(GLFWwindow *glfw_window, int width,
                                      int height) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW framebuffer resize failed");
        return;
    }

    glm_ivec2_copy((ivec2){width, height}, window->fb_size);
    int error = renderer_on_resize(window->renderer, width, height);
    if (error)
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Renderer resize failed");
}

static void key_callback(GLFWwindow *glfw_window, int key, int scancode,
                         int action, int mods) {
    (void)scancode;
    (void)mods;

    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW getting key failed");
        return;
    }

    window_input_key(window, key, action);
}

static void mouse_button_callback(GLFWwindow *glfw_window, int button,
                                  int action, int mods) {
    (void)mods;

    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW getting mouse button failed");
        return;
    }

    window_input_mouse_button_set(window, button, action);
}

static void scroll_callback(GLFWwindow *glfw_window, double offset_x,
                            double offset_y) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW getting mouse button failed");
        return;
    }

    window_input_scroll_offset_set(window,
                                   (vec2){(float)offset_x, (float)offset_y});
}

static void cursor_pos_callback(GLFWwindow *glfw_window, double pos_x,
                                double pos_y) {
    struct window *window =
        (struct window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "GLFW getting cursor pos failed");
        return;
    }

    window_input_cursor_pos_set(window, (vec2){(float)pos_x, (float)pos_y});
}

static int input_create(struct input **out, struct allocator *alloc) {
    if (!out || !alloc)
        return CLS_NULLPTR;

    return allocator_alloc((void **)out, alloc, sizeof(struct input),
                           alignof(struct input));
}

static int timing_create(struct timing **out, struct allocator *alloc) {
    if (!out || !alloc)
        return CLS_NULLPTR;

    struct timing *timing = NULL;
    int error = allocator_alloc((void **)&timing, alloc, sizeof(struct timing),
                                alignof(struct timing));
    if (error)
        return error;

    timing->last_frame = glfwGetTime();
    *out = timing;
    return CLS_SUCCESS;
}

int window_create(struct window **out, struct allocator *allocator_persistant,
                  struct allocator *allocator_frame, struct gfx_api *api,
                  ivec2 size, const char *title, bool vsync, ivec4 bg_color) {
    if (!out || !allocator_persistant || !allocator_frame || !title)
        return CLS_NULLPTR;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s", "Init GLFW failed");
        return CLS_GLFW;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    struct window *window = NULL;
    int error = allocator_alloc((void **)&window, allocator_persistant,
                                sizeof(struct window), alignof(struct window));
    if (error)
        return error;

    window->glfw_window = glfwCreateWindow(size[0], size[1], title, NULL, NULL);
    if (!window->glfw_window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "Creating GLFW window failed");
        glfwTerminate();
        error = CLS_GLFW;
        goto cleanup;
    }

    glfwMakeContextCurrent(window->glfw_window);
    glfwSetWindowUserPointer(window->glfw_window, window);
    glfwSetCursorPosCallback(window->glfw_window, cursor_pos_callback);
    glfwSetWindowSizeCallback(window->glfw_window, window_size_callback);
    glfwSetFramebufferSizeCallback(window->glfw_window,
                                   framebuffer_size_callback);
    glfwSetKeyCallback(window->glfw_window, key_callback);
    glfwSetMouseButtonCallback(window->glfw_window, mouse_button_callback);
    glfwSetScrollCallback(window->glfw_window, scroll_callback);
    glfwSwapInterval(vsync);

    error = input_create(&window->input, allocator_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window input failed");
        goto cleanup;
    }

    error = renderer_create(&window->renderer, allocator_persistant,
                            allocator_frame, api, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window renderer failed");
        goto cleanup;
    }

    error = timing_create(&window->timing, allocator_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window timing failed");
        goto cleanup;
    }

    glfwSwapBuffers(window->glfw_window);
    glfwGetFramebufferSize(window->glfw_window, &window->fb_size[0],
                           &window->fb_size[1]);
    *out = window;
    return CLS_SUCCESS;

cleanup:
    window_destroy(window);
    return error;
}

void window_destroy(struct window *in) {
    if (!in)
        return;

    glfwDestroyWindow(in->glfw_window);
    renderer_destroy(in->renderer);
}

int window_update(bool *out, struct window *in) {
    int error = window_should_close(out, in);
    if (error)
        return error;

    error = window_timing_update(in);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Updating window time failed");
        return error;
    }

    error = window_input_update(in);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Updating window input failed");
        return error;
    }

    return CLS_SUCCESS;
}

int window_renderer_update(struct window *in, struct app *app) {
    int error = renderer_frame_create(in->renderer, app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Renderer creating frame failed");
        return error;
    }

    error = renderer_swap_buffers(in->renderer, in->glfw_window);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Renderer swapping buffers failed");
        return error;
    }

    return CLS_SUCCESS;
}

int window_should_close(bool *out, struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = glfwWindowShouldClose(in->glfw_window);
    return CLS_SUCCESS;
}

int window_size_get(ivec2 out, struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    glm_ivec2_copy(in->win_size, out);
    return CLS_SUCCESS;
}

int window_fb_size_get(ivec2 out, struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    glm_ivec2_copy(in->fb_size, out);
    return CLS_SUCCESS;
}

int window_input_update(struct window *in) {
    if (!in)
        return CLS_NULLPTR;

    memcpy(in->input->last_keys, in->input->keys, sizeof(in->input->keys));
    memcpy(in->input->last_mouse_buttons, in->input->mouse_buttons,
           sizeof(in->input->mouse_buttons));

    glfwPollEvents();
    return CLS_SUCCESS;
}

int window_input_key(struct window *in, int key, int action) {
    if (!in)
        return CLS_NULLPTR;

    if (action == GLFW_PRESS)
        in->input->keys[key] = true;

    if (action == GLFW_RELEASE)
        in->input->keys[key] = false;

    return CLS_SUCCESS;
}

int window_input_mouse_button_set(struct window *in, int button, int action) {
    if (!in)
        return CLS_NULLPTR;

    if (button < 0 || button >= WINDOW_INPUT_MOUSE_BUTTONS_SIZE)
        return CLS_INVALID_ARG;

    if (action == GLFW_PRESS)
        in->input->mouse_buttons[button] = true;

    if (action == GLFW_RELEASE)
        in->input->mouse_buttons[button] = false;

    return CLS_SUCCESS;
}

int window_input_cursor_pos_get(vec2 out, struct window *in) {
    if (!in)
        return CLS_NULLPTR;

    glm_vec2_copy(in->input->cursor_pos, out);
    return CLS_SUCCESS;
}

int window_input_cursor_pos_set(struct window *in, vec2 pos) {
    if (!in)
        return CLS_NULLPTR;

    glm_vec2_copy(pos, in->input->cursor_pos);
    return CLS_SUCCESS;
}

int window_input_scroll_offset_get(vec2 out, struct window *in) {
    if (!in)
        return CLS_NULLPTR;

    glm_vec2_copy(in->input->scroll_offset, out);
    glm_vec2_copy((vec2){0.0f, 0.0f}, in->input->scroll_offset);
    return CLS_SUCCESS;
}

int window_input_scroll_offset_set(struct window *in, vec2 offset) {
    if (!in)
        return CLS_NULLPTR;

    glm_vec2_copy(offset, in->input->scroll_offset);
    return CLS_SUCCESS;
}

int window_input_key_pressed(bool *out, const struct window *in, int key) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->input->keys[key] && !in->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_key_released(bool *out, const struct window *in, int key) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = !in->input->keys[key] && in->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_key_down(bool *out, const struct window *in, int key) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->input->keys[key] && in->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_mouse_pressed(bool *out, const struct window *in, int button) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->input->mouse_buttons[button] &&
           !in->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_input_mouse_released(bool *out, const struct window *in,
                                int button) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = !in->input->mouse_buttons[button] &&
           in->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_input_mouse_down(bool *out, const struct window *in, int button) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->input->mouse_buttons[button] &&
           in->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_timing_update(struct window *in) {
    if (!in)
        return CLS_NULLPTR;

    double now = glfwGetTime();
    in->timing->delta_time = (float)(now - in->timing->last_frame);
    in->timing->total += in->timing->delta_time;
    in->timing->last_frame = now;

    if (in->timing->delta_time > 0.0f)
        in->timing->fps = 1.0f / in->timing->delta_time;

    in->timing->frame_count++;
    in->timing->fps_timer += in->timing->delta_time;

    if (in->timing->fps_timer >= 1.0f) {
        in->timing->fps_avg =
            (float)in->timing->frame_count / in->timing->fps_timer;
        in->timing->frame_count = 0;
        in->timing->fps_timer = 0.0f;
    }

    return CLS_SUCCESS;
}

int window_timing_dt_get(float *out, const struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->timing->delta_time;
    return CLS_SUCCESS;
}

int window_timing_fps_get(float *out, const struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->timing->fps;
    return CLS_SUCCESS;
}

int window_timing_fps_avg_get(float *out, const struct window *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->timing->fps_avg;
    return CLS_SUCCESS;
}
