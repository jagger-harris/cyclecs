#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/comp/transform.h"
#include "core/util/arena.h"
#include <GLFW/glfw3.h>

typedef int (*api_init)(void);
typedef int (*api_swap_buffers)(GLFWwindow *);
typedef void (*api_on_resize)(int, int);
typedef void (*api_render_frame)(void);
typedef struct renderer renderer;

err renderer_new(renderer **out, arena *mem, api_init init,
                 api_swap_buffers swap, api_on_resize resize,
                 api_render_frame render_frame);
err renderer_use(renderer *in);
err renderer_swap_buffers(renderer *in, GLFWwindow *window);
err renderer_resize(renderer *in, int width, int height);
err renderer_render_frame(renderer *in);
err renderer_draw_enqueue(renderer *in, struct material *mat,
                          struct transform *tf);

#endif /* GFX_RENDERER_H */
