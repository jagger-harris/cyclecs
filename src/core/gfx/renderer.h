#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "core/app/assets.h"
#include "core/ecs/world.h"
#include "core/gfx/camera.h"
#include "core/util/array.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

typedef int (*api_init)(void);
typedef int (*api_swap_buffers)(GLFWwindow *);
typedef void (*api_on_resize)(int, int);
typedef int (*api_draw_frame)(struct assets *, struct camera *, struct array *,
                              struct array *, struct array *, struct array *);

struct renderer {
    struct camera camera;
    struct array opaque_draws_2d;
    struct array opaque_draws_3d;
    struct array transparent_draws_2d;
    struct array transparent_draws_3d;
    api_init init;
    api_swap_buffers swap;
    api_on_resize resize;
    api_draw_frame draw_frame;
};

int renderer_init(struct renderer *out, const float aspect_ratio,
                  const api_init init, const api_swap_buffers swap,
                  const api_on_resize resize, const api_draw_frame draw_frame);
void renderer_destroy(struct renderer *in);
int renderer_use(const struct renderer *in);
int renderer_swap_buffers(struct renderer *in, GLFWwindow *window);
int renderer_resize(struct renderer *in, int width, int height);
int renderer_camera_update(struct renderer *in);
int renderer_camera_set_pos(struct renderer *in, float pos_x, float pos_y,
                            float pos_z);
int renderer_camera_move(struct renderer *in, float pos_x, float pos_y,
                         float pos_z);
int renderer_draw_frame(struct renderer *in, struct assets *assets,
                        struct ecs_world *world);

#endif // GFX_RENDERER_H
