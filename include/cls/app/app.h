#ifndef APP_H
#define APP_H

#include "cglm/types.h"

struct assets;
struct ecs;
struct gfx_api;
struct allocator;
struct window;

struct app {
    struct assets *assets;
    struct ecs *ecs;
    struct gfx_api *api;
    struct arena *arena_persistant;
    struct arena *arena_frame;
    struct allocator *alloc_persistant;
    struct allocator *alloc_frame;
    struct window *window;
};

int app_init(struct app *out, struct gfx_api *api, ivec2 size,
             const char *title, ivec4 bg_color);
void app_destroy(struct app *in);
int app_run(struct app *in);

#endif // APP_H
