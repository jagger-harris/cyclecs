#include "base/game/game.h"
#include "core/app/app.h"
#include "core/util/logger.h"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_TITLE "C ECS OpenGL Game Engine"

int main(void) {
    struct game_state state = {0};
    struct app app = {0};
    int error = app_init(&app, &state, (ivec2){WIN_WIDTH, WIN_HEIGHT},
                         WIN_TITLE, (struct color){10, 15, 30, 255});
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app failed");
        goto cleanup;
    }

    error = game_init(&state, &app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init game failed");
        goto cleanup;
    }

    error = app_run(&app, game_update);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Running app failed");
        goto cleanup;
    }

    app_destroy(&app);
    return CORE_SUCCESS;

cleanup:
    LOGGER_LOG_ERROR(LOGGER_FATAL, error, "%s", "Fatal error");
    app_destroy(&app);
    return error;
}
