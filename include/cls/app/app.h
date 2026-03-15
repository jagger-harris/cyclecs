#ifndef CLS_APP_H
#define CLS_APP_H

#include <cglm/types.h>

struct assets;
struct ecs;
struct gfx_api;
struct allocator;
struct window;

struct app {
    struct assets *assets;
    struct ecs *ecs;
    struct gfx_api *api;
    struct arena *arena_perm;
    struct arena *arena_frame;
    struct allocator *alloc_perm;
    struct allocator *alloc_frame;
    struct window *window;
};

int app_init(struct app *app, struct gfx_api *api, ivec2 size,
             const char *title, ivec4 bg_color);
void app_destroy(struct app *app);
int app_run(struct app *app);

#endif // CLS_APP_H
