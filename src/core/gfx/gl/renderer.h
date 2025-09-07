#ifndef GFX_GL_RENDERER_H
#define GFX_GL_RENDERER_H

#include "core/app/assets.h"
#include "core/gfx/camera.h"
#include "core/util/array.h"
#include <GLFW/glfw3.h>

int gl_renderer_init(void);
int gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
int gl_renderer_draw_frame(struct assets *assets, struct camera *camera,
                           struct array *opaque_draws_2d,
                           struct array *transparent_draws_2d,
                           struct array *opaque_draws_3d,
                           struct array *transparent_draws_3d);

#endif // GFX_GL_RENDERER_H
