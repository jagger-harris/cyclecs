#ifndef GFX_GL_RENDERER_H
#define GFX_GL_RENDERER_H

#include <GLFW/glfw3.h>

struct app;
struct array;
struct color;
struct renderer_ctx;

int gl_renderer_init(void);
int gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
int gl_renderer_draw_frame(struct app *app, struct color bg_color,
                           struct array *draw_cmds);

#endif // GFX_GL_RENDERER_H
