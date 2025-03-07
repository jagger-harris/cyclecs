#ifndef APP_WINDOW_H
#define APP_WINDOW_H

#include <GLFW/glfw3.h>

typedef struct renderer renderer;
typedef struct window window;

int window_new(window *out, int width, int height, const char *title);
int window_delete(window *in);
int window_get_native_window(GLFWwindow *out, window *in);
int window_should_close(int *out, window *in);
int window_set_renderer(window *in, renderer *api);

#endif /* APP_WINDOW_H */
