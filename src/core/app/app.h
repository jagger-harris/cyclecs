#ifndef APP_H
#define APP_H

#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/arena.h"
#include "core/util/err.h"

typedef int (*game_init_fn)(struct assets *assets, struct ecs *ecs,
                            struct renderer *renderer);
typedef int (*game_update_fn)(void);

struct app {
    struct assets assets;
    struct ecs ecs;
    struct renderer renderer;
    struct window window;
};

err app_init(struct app *out, struct arena *mem, int width, int height,
             const char *title);
void app_destroy(struct app *in);
err app_run(struct app *in, const game_init_fn init,
            const game_update_fn update);

#endif // APP_H
