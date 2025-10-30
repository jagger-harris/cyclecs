#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "core/app/input.h"
#include "core/app/timing.h"
#include "core/gfx/renderer.h"

struct window {
    struct input input;
    struct renderer renderer;
    struct timing timing;
    ivec2 size;
    GLFWwindow *glfw_window;
};

int window_init(struct window *out, struct gfx_api *api, ivec2 size,
                const char *title, bool vsync, ivec4 bg_color);
void window_destroy(struct window *in);
int window_should_close(bool *out, struct window *in);
int window_size_get(ivec2 out, struct window *in);

#endif // APP_WINDOW_H
