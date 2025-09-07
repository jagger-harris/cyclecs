#include "base/game/game.h"
#include "core/app/app.h"
#include "core/util/logger.h"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_TITLE "C ECS OpenGL Game Engine"

int main(void) {
    LOGGER_LOG(LOGGER_INFO, "%s", "Starting app");

    struct app app = {0};
    int status = app_init(&app, WIN_WIDTH, WIN_HEIGHT, WIN_TITLE);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init app failed");
        goto cleanup;
    }

    struct game game = {0};
    status = game_init(&game, &app);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Init game failed");
        goto cleanup;
    }

    status = app_run(&app, game_run);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s", "Running app failed");
        goto cleanup;
    }

    return CORE_SUCCESS;

cleanup:
    LOGGER_LOG_ERROR(LOGGER_FATAL, status, "%s", "Fatal error");
    app_destroy(&app);
    return status;
}
