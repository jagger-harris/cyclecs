#ifndef CLS_GL_RENDERER_H
#define CLS_GL_RENDERER_H

#include <GLFW/glfw3.h>
#include <cglm/types.h>

struct cls_app;
struct cls_array;
struct cls_renderer_ctx;

int cls_gl_renderer_init(ivec4 bg_color);
int cls_gl_renderer_swap_buffers(GLFWwindow *win);
void cls_gl_renderer_on_resize(int width, int height);
int cls_gl_renderer_draw_frame(struct cls_app *app,
                               struct cls_array *transparent_cmds,
                               struct cls_array *draw_cmds);

#endif // CLS_GL_RENDERER_H
