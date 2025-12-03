#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "cglm/types.h"
#include <stdbool.h>

#define WINDOW_INPUT_KEYS_SIZE (GLFW_KEY_LAST + 1)
#define WINDOW_INPUT_MOUSE_BUTTONS_SIZE (GLFW_MOUSE_BUTTON_LAST + 1)

typedef struct GLFWwindow GLFWwindow;
struct allocator;
struct app;
struct gfx_api;
struct input;
struct renderer;
struct timing;
struct window;

int window_create(struct window **out, struct allocator *allocator_persistant,
                  struct allocator *allocator_frame, struct gfx_api *api,
                  ivec2 size, const char *title, bool vsync, ivec4 bg_color);
void window_destroy(struct window *in);
int window_update(bool *out, struct window *in);
int window_renderer_update(struct window *in, struct app *app);
int window_should_close(bool *out, struct window *in);
int window_size_get(ivec2 out, struct window *in);
int window_fb_size_get(ivec2 out, struct window *in);
int window_input_update(struct window *in);
int window_input_key(struct window *in, int key, int action);
int window_input_mouse_button_set(struct window *in, int button, int action);
int window_input_cursor_pos_get(vec2 out, struct window *in);
int window_input_cursor_pos_set(struct window *in, vec2 pos);
int window_input_scroll_offset_get(vec2 out, struct window *in);
int window_input_scroll_offset_set(struct window *in, vec2 offset);
int window_input_key_pressed(bool *out, const struct window *in, int key);
int window_input_key_released(bool *out, const struct window *in, int key);
int window_input_key_down(bool *out, const struct window *in, int key);
int window_input_mouse_pressed(bool *out, const struct window *in, int button);
int window_input_mouse_released(bool *out, const struct window *in, int button);
int window_input_mouse_down(bool *out, const struct window *in, int button);
int window_timing_update(struct window *in);
int window_timing_dt_get(float *out, const struct window *in);
int window_timing_fps_get(float *out, const struct window *in);
int window_timing_fps_avg_get(float *out, const struct window *in);

#endif // APP_WINDOW_H
