#ifndef APP_H
#define APP_H

#include "core/app/app_time.h"
#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/ecs.h"
#include "core/util/arena.h"

struct app {
    struct window window;
    struct assets assets;
    struct ecs ecs;
    struct arena arena;
    struct app_time time;
};

typedef int (*game_update_fn)(struct app *);

int app_init(struct app *out, int width, int height, const char *title);
void app_destroy(struct app *in);
int app_run(struct app *in, const game_update_fn game_update);

#endif // APP_H
