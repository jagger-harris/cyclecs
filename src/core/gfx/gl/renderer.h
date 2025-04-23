#ifndef GFX_GL_RENDERER_H
#define GFX_GL_RENDERER_H

#include "core/util/err.h"
#include <GLFW/glfw3.h>

err gl_renderer_init(void);
err gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
void gl_renderer_render_frame(void);

#endif /* GFX_GL_RENDERER_H */
