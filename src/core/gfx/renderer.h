#ifndef GFX_RENDERER_H
#define GFX_RENDERER_H

#include "cglm/types.h"
#include "core/util/arena.h"
#include "core/util/array.h"
#include "core/util/table.h"

#define RENDERER_START_CMD_CAPACITY 128

struct app;
struct gfx_api;
typedef struct GLFWwindow GLFWwindow;

struct renderer {
    struct arena arena;
    struct gfx_api *api;
    ivec4 bg_color;
    struct array batches;
    struct table batch_indices;
    struct table cameras;
};

int renderer_init(struct renderer *out, struct gfx_api *api, ivec4 bg_color);
void renderer_destroy(struct renderer *in);
int renderer_swap_buffers(struct renderer *in, GLFWwindow *window);
int renderer_on_resize(struct renderer *in, int width, int height);
int renderer_draw_frame(struct renderer *in, struct app *app);

#endif // GFX_RENDERER_H
