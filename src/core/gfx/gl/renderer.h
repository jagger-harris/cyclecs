#ifndef GFX_GL_RENDERER_H
#define GFX_GL_RENDERER_H

#include <GLFW/glfw3.h>
#include <cglm/types.h>

struct app;
struct array;
struct renderer_ctx;

int gl_renderer_init(ivec4 bg_color);
int gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
int gl_renderer_draw_frame(struct app *app, struct array *draw_cmds);

#endif // GFX_GL_RENDERER_H
