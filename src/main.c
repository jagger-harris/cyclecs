#include "core/app/app.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>

int main(void) {
    int error = CORE_ERROR_SUCCESS;
    app *app = NULL;

    error = app_new(app, 1280, 720, "OpenGL Window");
    if (error) {
        logger_log(LOGGER_LOG_LEVEL_FATAL, "failed to make new app", error);
        exit(EXIT_FAILURE);
    }

    error = app_run(app);
    if (error) {
        logger_log(LOGGER_LOG_LEVEL_FATAL, "failed to run app", error);
        exit(EXIT_FAILURE);
    }

    error = app_delete(app);
    if (error) {
        logger_log(LOGGER_LOG_LEVEL_FATAL, "failed to delete app", error);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
