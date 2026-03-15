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
    float dt;
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

static int input_create(struct input **in, struct allocator *alloc) {
    if (!in || !alloc)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = allocator_alloc(&instance_ptr, alloc, sizeof(struct input),
                                alignof(struct input));
    if (error)
        return error;

    *in = instance_ptr;
    return CLS_SUCCESS;
}

static int timing_create(struct timing **t, struct allocator *alloc) {
    if (!t || !alloc)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = allocator_alloc(&instance_ptr, alloc, sizeof(struct timing),
                                alignof(struct timing));
    if (error)
        return error;

    struct timing *instance = instance_ptr;
    instance->last_frame = glfwGetTime();

    *t = instance;
    return CLS_SUCCESS;
}

int window_create(struct window **win, struct allocator *allocator_persistant,
                  struct allocator *allocator_frame, struct gfx_api *api,
                  ivec2 size, const char *title, bool vsync, ivec4 bg_color) {
    if (!win || !allocator_persistant || !allocator_frame || !title)
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

    struct window *instance = NULL;
    int error = allocator_alloc((void **)&instance, allocator_persistant,
                                sizeof(struct window), alignof(struct window));
    if (error)
        return error;

    instance->glfw_window =
        glfwCreateWindow(size[0], size[1], title, NULL, NULL);
    if (!instance->glfw_window) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_GLFW, "%s",
                         "Creating GLFW window failed");
        glfwTerminate();
        error = CLS_GLFW;
        goto cleanup;
    }

    glfwMakeContextCurrent(instance->glfw_window);
    glfwSetWindowUserPointer(instance->glfw_window, instance);
    glfwSetCursorPosCallback(instance->glfw_window, cursor_pos_callback);
    glfwSetWindowSizeCallback(instance->glfw_window, window_size_callback);
    glfwSetFramebufferSizeCallback(instance->glfw_window,
                                   framebuffer_size_callback);
    glfwSetKeyCallback(instance->glfw_window, key_callback);
    glfwSetMouseButtonCallback(instance->glfw_window, mouse_button_callback);
    glfwSetScrollCallback(instance->glfw_window, scroll_callback);
    glfwSwapInterval(vsync);

    error = input_create(&instance->input, allocator_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window input failed");
        goto cleanup;
    }

    error = renderer_create(&instance->renderer, allocator_persistant,
                            allocator_frame, api, bg_color);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window renderer failed");
        goto cleanup;
    }

    error = timing_create(&instance->timing, allocator_persistant);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Creating window timing failed");
        goto cleanup;
    }

    glfwSwapBuffers(instance->glfw_window);
    glfwGetFramebufferSize(instance->glfw_window, &instance->fb_size[0],
                           &instance->fb_size[1]);
    *win = instance;
    return CLS_SUCCESS;

cleanup:
    window_destroy(instance);
    return error;
}

void window_destroy(struct window *win) {
    if (!win)
        return;

    glfwDestroyWindow(win->glfw_window);
    renderer_destroy(win->renderer);
}

int window_update(bool *should_close, struct window *win) {
    int error = window_should_close(should_close, win);
    if (error)
        return error;

    error = window_timing_update(win);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Updating window time failed");
        return error;
    }

    error = window_input_update(win);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Updating window input failed");
        return error;
    }

    return CLS_SUCCESS;
}

int window_renderer_update(struct window *win, struct app *app) {
    int error = renderer_frame_create(win->renderer, app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Renderer creating frame failed");
        return error;
    }

    error = renderer_swap_buffers(win->renderer, win->glfw_window);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Renderer swapping buffers failed");
        return error;
    }

    return CLS_SUCCESS;
}

int window_should_close(bool *should_close, const struct window *win) {
    if (!should_close || !win)
        return CLS_NULLPTR;

    *should_close = glfwWindowShouldClose(win->glfw_window);
    return CLS_SUCCESS;
}

int window_size_get(ivec2 size, struct window *win) {
    if (!size || !win)
        return CLS_NULLPTR;

    glm_ivec2_copy(win->win_size, size);
    return CLS_SUCCESS;
}

int window_fb_size_get(ivec2 fb_size, struct window *win) {
    if (!fb_size || !win)
        return CLS_NULLPTR;

    glm_ivec2_copy(win->fb_size, fb_size);
    return CLS_SUCCESS;
}

int window_input_update(struct window *win) {
    if (!win)
        return CLS_NULLPTR;

    memcpy(win->input->last_keys, win->input->keys, sizeof(win->input->keys));
    memcpy(win->input->last_mouse_buttons, win->input->mouse_buttons,
           sizeof(win->input->mouse_buttons));

    glfwPollEvents();
    return CLS_SUCCESS;
}

int window_input_key(struct window *win, int key, int action) {
    if (!win)
        return CLS_NULLPTR;

    if (action == GLFW_PRESS)
        win->input->keys[key] = true;

    if (action == GLFW_RELEASE)
        win->input->keys[key] = false;

    return CLS_SUCCESS;
}

int window_input_mouse_button_set(struct window *win, int button, int action) {
    if (!win)
        return CLS_NULLPTR;

    if (button < 0 || button >= WINDOW_INPUT_MOUSE_BUTTONS_SIZE)
        return CLS_INVALID_ARG;

    if (action == GLFW_PRESS)
        win->input->mouse_buttons[button] = true;

    if (action == GLFW_RELEASE)
        win->input->mouse_buttons[button] = false;

    return CLS_SUCCESS;
}

int window_input_cursor_pos_get(vec2 pos, struct window *win) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(win->input->cursor_pos, pos);
    return CLS_SUCCESS;
}

int window_input_cursor_pos_set(struct window *win, vec2 pos) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(pos, win->input->cursor_pos);
    return CLS_SUCCESS;
}

int window_input_scroll_offset_get(vec2 offset, struct window *win) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(win->input->scroll_offset, offset);
    glm_vec2_copy((vec2){0.0f, 0.0f}, win->input->scroll_offset);
    return CLS_SUCCESS;
}

int window_input_scroll_offset_set(struct window *win, vec2 offset) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(offset, win->input->scroll_offset);
    return CLS_SUCCESS;
}

int window_input_key_pressed(bool *pressed, const struct window *win, int key) {
    if (!pressed || !win)
        return CLS_NULLPTR;

    *pressed = win->input->keys[key] && !win->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_key_released(bool *released, const struct window *win,
                              int key) {
    if (!released || !win)
        return CLS_NULLPTR;

    *released = !win->input->keys[key] && win->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_key_down(bool *down, const struct window *win, int key) {
    if (!down || !win)
        return CLS_NULLPTR;

    *down = win->input->keys[key] && win->input->last_keys[key];
    return CLS_SUCCESS;
}

int window_input_mouse_pressed(bool *pressed, const struct window *win,
                               int button) {
    if (!pressed || !win)
        return CLS_NULLPTR;

    *pressed = win->input->mouse_buttons[button] &&
               !win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_input_mouse_released(bool *released, const struct window *win,
                                int button) {
    if (!released || !win)
        return CLS_NULLPTR;

    *released = !win->input->mouse_buttons[button] &&
                win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_input_mouse_down(bool *down, const struct window *win, int button) {
    if (!down || !win)
        return CLS_NULLPTR;

    *down = win->input->mouse_buttons[button] &&
            win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int window_timing_update(struct window *win) {
    if (!win)
        return CLS_NULLPTR;

    double now = glfwGetTime();
    win->timing->dt = (float)(now - win->timing->last_frame);
    win->timing->total += win->timing->dt;
    win->timing->last_frame = now;

    if (win->timing->dt > 0.0f)
        win->timing->fps = 1.0f / win->timing->dt;

    win->timing->frame_count++;
    win->timing->fps_timer += win->timing->dt;

    if (win->timing->fps_timer >= 1.0f) {
        win->timing->fps_avg =
            (float)win->timing->frame_count / win->timing->fps_timer;
        win->timing->frame_count = 0;
        win->timing->fps_timer = 0.0f;
    }

    return CLS_SUCCESS;
}

int window_timing_dt_get(float *dt, const struct window *win) {
    if (!dt || !win)
        return CLS_NULLPTR;

    *dt = win->timing->dt;
    return CLS_SUCCESS;
}

int window_timing_fps_get(float *fps, const struct window *win) {
    if (!fps || !win)
        return CLS_NULLPTR;

    *fps = win->timing->fps;
    return CLS_SUCCESS;
}

int window_timing_fps_avg_get(float *fps_avg, const struct window *win) {
    if (!fps_avg || !win)
        return CLS_NULLPTR;

    *fps_avg = win->timing->fps_avg;
    return CLS_SUCCESS;
}
