#ifndef GRAPHICS_GL_RENDERER_H
#define GRAPHICS_GL_RENDERER_H

#include <GLFW/glfw3.h>

int gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
void gl_renderer_clear(void);

#endif /* GRAPHICS_GL_RENDERER_H */
