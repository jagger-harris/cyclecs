#ifndef APP_H
#define APP_H

#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/ecs.h"
#include "core/util/err.h"

struct app {
    struct assets assets;
    struct ecs ecs;
    struct window window;
};

typedef err (*game_init_fn)(struct app *);
typedef err (*game_update_fn)(struct app *);

err app_init(struct app *out, int width, int height, const char *title);
void app_destroy(struct app *in);
err app_run(struct app *in, const game_init_fn init,
            const game_update_fn update);

#endif // APP_H
