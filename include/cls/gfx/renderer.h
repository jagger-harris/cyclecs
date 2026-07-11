#ifndef CLS_RENDERER_H
#define CLS_RENDERER_H

#include <cglm/types.h>
#include <cls/ecs/component/components.h>
#include <cls/util/array.h>
#include <cls/util/mem.h>
#include <cls/util/table.h>

typedef struct GLFWwindow GLFWwindow;
struct cls_app;
struct cls_gfx_api;
struct cls_mem;
struct cls_renderer;
struct cls_ecs_world;
struct renderable;
struct transform;

struct cls_renderer_cmd {
    struct renderable ren;
    struct transform tf;
    float depth;
};

int cls_renderer_create(struct cls_renderer **rend, struct cls_mem *mem_perm,
                        struct cls_mem *mem_frame, struct cls_gfx_api *api,
                        const ivec4 bg_color);
void cls_renderer_destroy(struct cls_renderer *rend);
int cls_renderer_swap_buffers(struct cls_renderer *rend, GLFWwindow *win);
int cls_renderer_on_resize(struct cls_renderer *rend, int width, int height);
int cls_renderer_cmd_push(struct cls_renderer *rend, struct renderable *ren,
                          struct transform *tf, float depth);
int cls_renderer_frame_create(struct cls_renderer *rend, struct cls_app *app);

#endif // CLS_RENDERER_H
