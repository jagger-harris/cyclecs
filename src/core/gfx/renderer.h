#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "cglm/types.h"
#include "core/util/allocator.h"
#include "core/util/array.h"
#include "core/util/table.h"

#define RENDERER_START_CMD_CAPACITY 128

typedef struct GLFWwindow GLFWwindow;
struct allocator;
struct app;
struct gfx_api;
struct renderer;

int renderer_create(struct renderer **out,
                    struct allocator *allocator_persistant,
                    struct allocator *allocator_frame, struct gfx_api *api,
                    ivec4 bg_color);
void renderer_destroy(struct renderer *in);
int renderer_swap_buffers(struct renderer *in, GLFWwindow *window);
int renderer_on_resize(struct renderer *in, int width, int height);
int renderer_draw_frame(struct renderer *in, struct app *app);

#endif // GFX_RENDERER_H
