#ifndef GRAPHICS_RENDERER_H
#define GRAPHICS_RENDERER_H

#include "../app/window.h"
#include <GLFW/glfw3.h>

typedef int (*api_swap_buffers)(GLFWwindow *);
typedef void (*api_on_resize)(int, int);
typedef void (*api_clear)(void);
typedef struct renderer renderer;

int renderer_init(renderer *out, api_swap_buffers swap, api_on_resize resize,
                  api_clear clear);
int renderer_swap_buffers(renderer *in, GLFWwindow *window);
int renderer_resize(renderer *in, int width, int height);
int renderer_clear(renderer *in);

#endif /* GRAPHICS_RENDERER_H */
