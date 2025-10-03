#ifndef APP_H
#define APP_H

#include "core/app/assets.h"
#include "core/app/ecs.h"
#include "core/app/event_queue.h"
#include "core/app/window.h"
#include "core/gfx/api.h"

struct app {
    struct assets assets;
    struct ecs ecs;
    struct event_queue event_queue;
    struct gfx_api api;
    struct window window;
    void *game_state;
};

typedef int (*game_update_fn)(struct app *app);

int app_init(struct app *out, void *game_state, ivec2 size, const char *title,
             struct color bg_color);
void app_destroy(struct app *in);
int app_run(struct app *in, const game_update_fn game_update);

#endif // APP_H
