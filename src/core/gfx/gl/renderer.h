#ifndef GFX_GL_RENDERER_H
#define GFX_GL_RENDERER_H

#include "core/app/assets.h"
#include "core/gfx/camera.h"
#include "core/util/array.h"
#include "core/util/err.h"
#include <GLFW/glfw3.h>

err gl_renderer_init(void);
err gl_renderer_swap_buffers(GLFWwindow *window);
void gl_renderer_on_resize(int width, int height);
err gl_renderer_render_frame(struct assets *assets, struct camera *camera,
                             struct array *opaque_draws,
                             struct array *transparent_draws);

#endif // GFX_GL_RENDERER_H
