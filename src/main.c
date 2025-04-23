#include "base/game/game.h"
#include "core/app/app.h"
#include "core/util/arena.h"
#include "core/util/err.h"
#include "core/util/logger.h"
#include <stdlib.h>

#define SCR_WIDTH 1280
#define SCR_HEIGHT 720
#define TITLE "C ECS OpenGL Game Engine"

err main(void) {
    err err = CORE_SUCCESS;
    app *app = NULL;
    arena *app_arena = NULL;

    logger_log(LOGGER_INFO, "Starting app", 0);

    err = arena_new(&app_arena, 1024 * 1024);
    if (err)
        goto err;

    err = app_new(&app, app_arena, SCR_WIDTH, SCR_HEIGHT, TITLE);
    if (err)
        goto err;

    err = app_run(app, game_init, game_run);
    if (err)
        goto err;

    exit(EXIT_SUCCESS);

err:
    logger_log(LOGGER_FATAL, "Critical state. Stopping...", err);
    app_delete(app);
    arena_delete(app_arena);
    exit(EXIT_FAILURE);
}
