#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "core/util/err.h"
#include <GLFW/glfw3.h>

struct window {
    GLFWwindow *native;
    struct renderer *renderer;
};

err window_init(struct window *out, int width, int height, const char *title,
                int vsync);
void window_destroy(struct window *in);

#endif // APP_WINDOW_H
