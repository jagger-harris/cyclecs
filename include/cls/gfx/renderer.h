#ifndef CLS_RENDERER_H
#define CLS_RENDERER_H

#include <cglm/types.h>
#include <cls/util/allocator.h>
#include <cls/util/array.h>
#include <cls/util/table.h>

typedef struct GLFWwindow GLFWwindow;
struct allocator;
struct app;
struct gfx_api;
struct renderer;

int renderer_create(struct renderer **rend, struct allocator *alloc_perm,
                    struct allocator *alloc_frame, struct gfx_api *api,
                    ivec4 bg_color);
void renderer_destroy(struct renderer *rend);
int renderer_swap_buffers(struct renderer *rend, GLFWwindow *win);
int renderer_on_resize(struct renderer *rend, int width, int height);
int renderer_frame_create(struct renderer *rend, struct app *app);

#endif // CLS_RENDERER_H
