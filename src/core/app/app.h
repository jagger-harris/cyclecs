#ifndef APP_H
#define APP_H

#include "cglm/types.h"

struct assets;
struct ecs;
struct gfx_api;
struct mem;
struct window;

struct app {
    struct assets *assets;
    struct ecs *ecs;
    struct gfx_api *api;
    struct arena *arena_persistant;
    struct arena *arena_frame;
    struct mem *mem_persistant;
    struct mem *mem_frame;
    struct window *window;
    void *game_state;
};

typedef int (*game_update_fn)(struct app *app);

int app_init(struct app *out, struct gfx_api *api, void *game_state, ivec2 size,
             const char *title, ivec4 bg_color);
void app_destroy(struct app *in);
int app_run(struct app *in, const game_update_fn game_update);

#endif // APP_H
