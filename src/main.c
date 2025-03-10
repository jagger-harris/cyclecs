#include "core/app/app.h"
#include "core/util/arena.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <stdlib.h>

#define SCR_WIDTH 1280
#define SCR_HEIGHT 720
#define TITLE "OpenGL Window"
#define VSYNC 1

int main(void) {
    int error = CORE_SUCCESS;
    app *app = NULL;
    arena *app_arena = NULL;

    logger_log(LOGGER_INFO, "Starting app", 0);

    error = arena_new(&app_arena, 1024 * 1024);
    if (error)
        goto error;

    error = app_new(&app, app_arena, SCR_WIDTH, SCR_HEIGHT, TITLE);
    if (error)
        goto cleanup;

    error = app_run(app);
    if (error)
        goto cleanup;

    exit(EXIT_SUCCESS);

cleanup:
    error = app_delete(app);
    if (error)
        goto error;

    error = arena_delete(app_arena);
    if (error)
        goto error;

error:
    logger_log(LOGGER_FATAL, "Critical state. Stopping...", error);
    exit(EXIT_FAILURE);
}
