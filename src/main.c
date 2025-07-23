#include "base/game/game.h"
#include "core/app/app.h"
#include "core/util/err.h"
#include "core/util/logger.h"
#include <stdlib.h>

#define MEM_SIZE 1024 * 1024
#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_TITLE "C ECS OpenGL Game Engine"

err main(void) {
    err status = CORE_SUCCESS;
    struct app app = {0};

    logger_log(LOGGER_INFO, "Starting app");

    status = app_init(&app, WIN_WIDTH, WIN_HEIGHT, WIN_TITLE);
    if (status)
        goto err;

    status = app_run(&app, game_init, game_run);
    if (status)
        goto err;

    exit(EXIT_SUCCESS);

err:
    logger_log_err(LOGGER_FATAL, status, "Fatal error");
    app_destroy(&app);
    exit(EXIT_FAILURE);
}
