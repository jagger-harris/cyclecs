#ifndef CLS_WINDOW_H
#define CLS_WINDOW_H

#include <cglm/types.h>
#include <stdbool.h>

#define WINDOW_INPUT_KEYS_SIZE (GLFW_KEY_LAST + 1)
#define WINDOW_INPUT_MOUSE_BUTTONS_SIZE (GLFW_MOUSE_BUTTON_LAST + 1)

struct cls_app;
struct cls_gfx_api;
struct cls_input;
struct cls_mem;
struct cls_renderer;
struct cls_timing;
struct cls_window;
typedef struct GLFWwindow GLFWwindow;

int cls_window_create(struct cls_window **win, struct cls_mem *alloc_perm,
                      struct cls_mem *alloc_frame, struct cls_gfx_api *api,
                      ivec2 size, const char *title, bool vsync,
                      ivec4 bg_color);
void cls_window_destroy(struct cls_window *win);
int cls_window_update(bool *should_close, struct cls_window *win);
int cls_window_renderer_update(struct cls_window *win, struct cls_app *app);
int cls_window_should_close(bool *should_close, const struct cls_window *win);
int cls_window_size_get(ivec2 win_size, struct cls_window *win);
int cls_window_fb_size_get(ivec2 fb_size, struct cls_window *win);
int cls_window_input_update(struct cls_window *win);
int cls_window_input_key(struct cls_window *win, int key, int action);
int cls_window_input_mouse_button_set(struct cls_window *win, int button,
                                      int action);
int cls_window_input_cursor_pos_get(vec2 pos, struct cls_window *win);
int cls_window_input_cursor_pos_set(struct cls_window *win, vec2 pos);
int cls_window_input_scroll_offset_get(vec2 offset, struct cls_window *win);
int cls_window_input_scroll_offset_set(struct cls_window *win, vec2 offset);
int cls_window_input_key_pressed(bool *pressed, const struct cls_window *win,
                                 int key);
int cls_window_input_key_released(bool *released, const struct cls_window *win,
                                  int key);
int cls_window_input_key_down(bool *down, const struct cls_window *win,
                              int key);
int cls_window_input_mouse_pressed(bool *pressed, const struct cls_window *win,
                                   int button);
int cls_window_input_mouse_released(bool *released,
                                    const struct cls_window *win, int button);
int cls_window_input_mouse_down(bool *down, const struct cls_window *win,
                                int button);
int cls_window_timing_update(struct cls_window *win);
int cls_window_timing_dt_get(float *dt, const struct cls_window *win);
int cls_window_timing_fps_get(float *fps, const struct cls_window *win);
int cls_window_timing_fps_avg_get(float *fps_avg, const struct cls_window *win);

#endif // CLS_WINDOW_H
