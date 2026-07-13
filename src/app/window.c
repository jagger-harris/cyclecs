#include <GLFW/glfw3.h>
#include <cglm/ivec2.h>
#include <cglm/vec2.h>
#include <cls/app/window.h>
#include <cls/gfx/renderer.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/mem.h>
#include <string.h>

enum {
    WINDOW_INPUT_KEYS_SIZE = GLFW_KEY_LAST + 1,
    WINDOW_INPUT_MOUSE_BUTTONS_SIZE = GLFW_MOUSE_BUTTON_LAST + 1,
};

struct cls_input {
    bool keys[WINDOW_INPUT_KEYS_SIZE];
    bool last_keys[WINDOW_INPUT_KEYS_SIZE];
    bool mouse_buttons[WINDOW_INPUT_MOUSE_BUTTONS_SIZE];
    bool last_mouse_buttons[WINDOW_INPUT_MOUSE_BUTTONS_SIZE];
    vec2 cursor_pos;
    vec2 scroll_offset;
};

struct cls_timing {
    float dt;
    float total;
    double last_frame;
    int frame_count;
    float fps;
    float fps_avg;
    float fps_timer;
};

struct cls_window {
    struct cls_input *input;
    struct cls_renderer *renderer;
    struct cls_timing *timing;
    ivec2 win_size;
    ivec2 fb_size;
    GLFWwindow *glfw_window;
};

static void error_callback(int error, const char *msg) {
    CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "GLFW error: %s", msg);
}

static void window_size_callback(GLFWwindow *glfw_window, int width,
                                 int height) {
    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW window resize failed");
        return;
    }

    glm_ivec2_copy((ivec2){width, height}, window->win_size);
}

static void framebuffer_size_callback(GLFWwindow *glfw_window, int width,
                                      int height) {
    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW framebuffer resize failed");
        return;
    }

    glm_ivec2_copy((ivec2){width, height}, window->fb_size);
    int error = cls_renderer_on_resize(window->renderer, width, height);
    if (error)
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Renderer resize failed");
}

static void key_callback(GLFWwindow *glfw_window, int key, int scancode,
                         int action, int mods) {
    (void)scancode;
    (void)mods;

    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW getting key failed");
        return;
    }

    cls_window_input_key(window, key, action);
}

static void mouse_button_callback(GLFWwindow *glfw_window, int button,
                                  int action, int mods) {
    (void)mods;

    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW getting mouse button failed");
        return;
    }

    cls_window_input_mouse_button_set(window, button, action);
}

static void scroll_callback(GLFWwindow *glfw_window, double offset_x,
                            double offset_y) {
    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW getting mouse button failed");
        return;
    }

    cls_window_input_scroll_offset_set(
        window, (vec2){(float)offset_x, (float)offset_y});
}

static void cursor_pos_callback(GLFWwindow *glfw_window, double pos_x,
                                double pos_y) {
    struct cls_window *window =
        (struct cls_window *)glfwGetWindowUserPointer(glfw_window);

    if (!window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "GLFW getting cursor pos failed");
        return;
    }

    cls_window_input_cursor_pos_set(window, (vec2){(float)pos_x, (float)pos_y});
}

static int input_create(struct cls_input **in, struct cls_mem *alloc) {
    if (!in || !alloc)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_mem_alloc(&instance_ptr, alloc, sizeof(struct cls_input),
                              alignof(struct cls_input));
    if (error)
        return error;

    memset(instance_ptr, 0, sizeof(struct cls_input));
    *in = instance_ptr;
    return CLS_SUCCESS;
}

static int timing_create(struct cls_timing **t, struct cls_mem *alloc) {
    if (!t || !alloc)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_mem_alloc(&instance_ptr, alloc, sizeof(struct cls_timing),
                              alignof(struct cls_timing));
    if (error)
        return error;

    struct cls_timing *instance = instance_ptr;
    instance->last_frame = glfwGetTime();

    *t = instance;
    return CLS_SUCCESS;
}

int cls_window_create(struct cls_window **win, struct cls_mem *mem_persistant,
                      struct cls_mem *mem_frame, struct cls_gfx_api *api,
                      ivec2 size, const char *title, bool vsync,
                      const ivec4 bg_color) {
    if (!win || !mem_persistant || !mem_frame || !title)
        return CLS_NULLPTR;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
                             "Init GLFW failed");
        return CLS_GLFW;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    struct cls_window *instance = NULL;
    int error =
        cls_mem_alloc((void **)&instance, mem_persistant,
                      sizeof(struct cls_window), alignof(struct cls_window));
    if (error)
        return error;

    instance->glfw_window =
        glfwCreateWindow(size[0], size[1], title, NULL, NULL);
    if (!instance->glfw_window) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_GLFW, "%s",
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

    error = input_create(&instance->input, mem_persistant);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating window input failed");
        goto cleanup;
    }

    error = cls_renderer_create(&instance->renderer, mem_persistant, mem_frame,
                                api, bg_color);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating window renderer failed");
        goto cleanup;
    }

    error = timing_create(&instance->timing, mem_persistant);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Creating window timing failed");
        goto cleanup;
    }

    glfwSwapBuffers(instance->glfw_window);
    glfwGetFramebufferSize(instance->glfw_window, &instance->fb_size[0],
                           &instance->fb_size[1]);
    *win = instance;
    return CLS_SUCCESS;

cleanup:
    cls_window_destroy(instance);
    return error;
}

void cls_window_destroy(struct cls_window *win) {
    if (!win)
        return;

    glfwDestroyWindow(win->glfw_window);
    cls_renderer_destroy(win->renderer);
}

int cls_window_renderer_get(struct cls_renderer **rend,
                            struct cls_window *win) {
    if (!rend || !win)
        return CLS_NULLPTR;

    *rend = win->renderer;
    return CLS_SUCCESS;
}

int cls_window_update(bool *should_close, struct cls_window *win) {
    int error = cls_window_should_close(should_close, win);
    if (error)
        return error;

    error = cls_window_timing_update(win);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Updating window time failed");
        return error;
    }

    error = cls_window_input_update(win);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Updating window input failed");
        return error;
    }

    return CLS_SUCCESS;
}

int cls_window_renderer_update(struct cls_window *win, struct cls_app *app) {
    if (!win || !app)
        return CLS_NULLPTR;

    int error = cls_renderer_frame_create(win->renderer, app);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Renderer creating frame failed");
        return error;
    }

    error = cls_renderer_swap_buffers(win->renderer, win->glfw_window);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Renderer swapping buffers failed");
        return error;
    }

    return CLS_SUCCESS;
}

int cls_window_should_close(bool *should_close, const struct cls_window *win) {
    if (!should_close || !win)
        return CLS_NULLPTR;

    *should_close = glfwWindowShouldClose(win->glfw_window);
    return CLS_SUCCESS;
}

int cls_window_size_get(ivec2 size, struct cls_window *win) {
    if (!size || !win)
        return CLS_NULLPTR;

    glm_ivec2_copy(win->win_size, size);
    return CLS_SUCCESS;
}

int cls_window_fb_size_get(ivec2 fb_size, struct cls_window *win) {
    if (!fb_size || !win)
        return CLS_NULLPTR;

    glm_ivec2_copy(win->fb_size, fb_size);
    return CLS_SUCCESS;
}

int cls_window_input_update(struct cls_window *win) {
    if (!win)
        return CLS_NULLPTR;

    memcpy(win->input->last_keys, win->input->keys, sizeof(win->input->keys));
    memcpy(win->input->last_mouse_buttons, win->input->mouse_buttons,
           sizeof(win->input->mouse_buttons));

    glfwPollEvents();
    return CLS_SUCCESS;
}

int cls_window_input_key(struct cls_window *win, int key, int action) {
    if (!win)
        return CLS_NULLPTR;

    if (key < 0 || key >= WINDOW_INPUT_KEYS_SIZE)
        return CLS_INVALID_ARG;

    if (action == GLFW_PRESS)
        win->input->keys[key] = true;

    if (action == GLFW_RELEASE)
        win->input->keys[key] = false;

    return CLS_SUCCESS;
}

int cls_window_input_mouse_button_set(struct cls_window *win, int button,
                                      int action) {
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

int cls_window_input_cursor_pos_get(vec2 pos, struct cls_window *win) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(win->input->cursor_pos, pos);
    return CLS_SUCCESS;
}

int cls_window_input_cursor_pos_set(struct cls_window *win, vec2 pos) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(pos, win->input->cursor_pos);
    return CLS_SUCCESS;
}

int cls_window_input_scroll_offset_get(vec2 offset, struct cls_window *win) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(win->input->scroll_offset, offset);
    return CLS_SUCCESS;
}

int cls_window_input_scroll_offset_set(struct cls_window *win, vec2 offset) {
    if (!win)
        return CLS_NULLPTR;

    glm_vec2_copy(offset, win->input->scroll_offset);
    return CLS_SUCCESS;
}

int cls_window_input_key_pressed(bool *pressed, const struct cls_window *win,
                                 int key) {
    if (!pressed || !win)
        return CLS_NULLPTR;

    *pressed = win->input->keys[key] && !win->input->last_keys[key];
    return CLS_SUCCESS;
}

int cls_window_input_key_released(bool *released, const struct cls_window *win,
                                  int key) {
    if (!released || !win)
        return CLS_NULLPTR;

    *released = !win->input->keys[key] && win->input->last_keys[key];
    return CLS_SUCCESS;
}

int cls_window_input_key_down(bool *down, const struct cls_window *win,
                              int key) {
    if (!down || !win)
        return CLS_NULLPTR;

    *down = win->input->keys[key] && win->input->last_keys[key];
    return CLS_SUCCESS;
}

int cls_window_input_mouse_pressed(bool *pressed, const struct cls_window *win,
                                   int button) {
    if (!pressed || !win)
        return CLS_NULLPTR;

    *pressed = win->input->mouse_buttons[button] &&
               !win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int cls_window_input_mouse_released(bool *released,
                                    const struct cls_window *win, int button) {
    if (!released || !win)
        return CLS_NULLPTR;

    *released = !win->input->mouse_buttons[button] &&
                win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int cls_window_input_mouse_down(bool *down, const struct cls_window *win,
                                int button) {
    if (!down || !win)
        return CLS_NULLPTR;

    *down = win->input->mouse_buttons[button] &&
            win->input->last_mouse_buttons[button];
    return CLS_SUCCESS;
}

int cls_window_timing_update(struct cls_window *win) {
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

int cls_window_timing_dt_get(float *dt, const struct cls_window *win) {
    if (!dt || !win)
        return CLS_NULLPTR;

    *dt = win->timing->dt;
    return CLS_SUCCESS;
}

int cls_window_timing_fps_get(float *fps, const struct cls_window *win) {
    if (!fps || !win)
        return CLS_NULLPTR;

    *fps = win->timing->fps;
    return CLS_SUCCESS;
}

int cls_window_timing_fps_avg_get(float *fps_avg,
                                  const struct cls_window *win) {
    if (!fps_avg || !win)
        return CLS_NULLPTR;

    *fps_avg = win->timing->fps_avg;
    return CLS_SUCCESS;
}
