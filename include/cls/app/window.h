#ifndef CLS_WINDOW_H
#define CLS_WINDOW_H

#include <cglm/types.h>
#include <stdbool.h>

#define WINDOW_INPUT_KEYS_SIZE (GLFW_KEY_LAST + 1)
#define WINDOW_INPUT_MOUSE_BUTTONS_SIZE (GLFW_MOUSE_BUTTON_LAST + 1)

struct allocator;
struct app;
struct gfx_api;
struct input;
struct renderer;
struct timing;
struct window;
typedef struct GLFWwindow GLFWwindow;

int window_create(struct window **win, struct allocator *alloc_perm,
                  struct allocator *alloc_frame, struct gfx_api *api,
                  ivec2 size, const char *title, bool vsync, ivec4 bg_color);
void window_destroy(struct window *win);
int window_update(bool *should_close, struct window *win);
int window_renderer_update(struct window *win, struct app *app);
int window_should_close(bool *should_close, const struct window *win);
int window_size_get(ivec2 win_size, struct window *win);
int window_fb_size_get(ivec2 fb_size, struct window *win);
int window_input_update(struct window *win);
int window_input_key(struct window *win, int key, int action);
int window_input_mouse_button_set(struct window *win, int button, int action);
int window_input_cursor_pos_get(vec2 pos, struct window *win);
int window_input_cursor_pos_set(struct window *win, vec2 pos);
int window_input_scroll_offset_get(vec2 offset, struct window *win);
int window_input_scroll_offset_set(struct window *win, vec2 offset);
int window_input_key_pressed(bool *pressed, const struct window *win, int key);
int window_input_key_released(bool *released, const struct window *win,
                              int key);
int window_input_key_down(bool *down, const struct window *win, int key);
int window_input_mouse_pressed(bool *pressed, const struct window *win,
                               int button);
int window_input_mouse_released(bool *released, const struct window *win,
                                int button);
int window_input_mouse_down(bool *down, const struct window *win, int button);
int window_timing_update(struct window *win);
int window_timing_dt_get(float *dt, const struct window *win);
int window_timing_fps_get(float *fps, const struct window *win);
int window_timing_fps_avg_get(float *fps_avg, const struct window *win);

#endif // CLS_WINDOW_H
