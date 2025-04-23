#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include "core/util/arena.h"
#include "core/util/err.h"
#include <GLFW/glfw3.h>

typedef struct renderer renderer;
typedef struct window window;

err window_new(window **out, arena *mem, int width, int height,
               const char *title, int vsync);
err window_delete(window *in);
err window_get_native_window(GLFWwindow **out, window *in);
err window_should_close(int *out, window *in);
err window_set_renderer(window *in, renderer *api);

#endif /* APP_WINDOW_H */
