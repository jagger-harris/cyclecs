#ifndef CLS_RENDERER_H
#define CLS_RENDERER_H

#include <cglm/types.h>
#include <cls/util/allocator.h>
#include <cls/util/array.h>
#include <cls/util/table.h>

typedef struct GLFWwindow GLFWwindow;
struct cls_cls_allocator;
struct cls_app;
struct cls_gfx_api;
struct cls_renderer;

int cls_renderer_create(struct cls_renderer **rend,
                        struct cls_allocator *alloc_perm,
                        struct cls_allocator *alloc_frame,
                        struct cls_gfx_api *api, ivec4 bg_color);
void cls_renderer_destroy(struct cls_renderer *rend);
int cls_renderer_swap_buffers(struct cls_renderer *rend, GLFWwindow *win);
int cls_renderer_on_resize(struct cls_renderer *rend, int width, int height);
int cls_renderer_frame_create(struct cls_renderer *rend, struct cls_app *app);

#endif // CLS_RENDERER_H
