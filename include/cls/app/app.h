#ifndef CLS_APP_H
#define CLS_APP_H

#include <cglm/types.h>

struct cls_assets;
struct cls_ecs;
struct cls_gfx_api;
struct cls_mem;
struct cls_window;

struct cls_app {
    struct cls_assets *assets;
    struct cls_ecs *ecs;
    struct cls_gfx_api *api;
    struct cls_arena *arena_perm;
    struct cls_arena *arena_frame;
    struct cls_mem *mem_perm;
    struct cls_mem *mem_frame;
    struct cls_window *window;
};

int cls_app_init(struct cls_app *app, struct cls_gfx_api *api, ivec2 size,
                 const char *title, const ivec4 bg_color);
void cls_app_destroy(struct cls_app *app);
int cls_app_run(struct cls_app *app);

#endif // CLS_APP_H
