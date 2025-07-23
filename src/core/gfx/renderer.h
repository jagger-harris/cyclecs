#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "core/app/assets.h"
#include "core/gfx/camera.h"
#include "core/gfx/draw_call.h"
#include "core/util/array.h"
#include <GLFW/glfw3.h>

typedef err (*api_init)(void);
typedef err (*api_swap_buffers)(GLFWwindow *);
typedef void (*api_on_resize)(int, int);
typedef err (*api_render_frame)(struct assets *, struct camera *,
                                struct array *, struct array *);

struct renderer {
    struct camera camera;
    struct array opaque_draws;
    struct array transparent_draws;
    api_init init;
    api_swap_buffers swap;
    api_on_resize resize;
    api_render_frame render_frame;
};

err renderer_init(struct renderer *out, const float aspect_ratio,
                  const api_init init, const api_swap_buffers swap,
                  const api_on_resize resize,
                  const api_render_frame render_frame);
void renderer_destroy(struct renderer *in);
err renderer_draw_call_add(struct renderer *in, struct draw_call *call);
err renderer_use(const struct renderer *in);
err renderer_swap_buffers(struct renderer *in, GLFWwindow *window);
err renderer_resize(struct renderer *in, int width, int height);
err renderer_camera_update(struct renderer *in);
err renderer_camera_set_pos(struct renderer *in, float pos_x, float pos_y,
                            float pos_z);
err renderer_camera_move(struct renderer *in, float pos_x, float pos_y,
                         float pos_z);
err renderer_render_frame(struct renderer *in, struct assets *assets);

#endif // GFX_RENDERER_H
