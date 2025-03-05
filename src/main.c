#include "core/ecs/ecs.h"
#include "core/util/logger.h"
#include <stdlib.h>

int main(void) {
    ecs *state = NULL;

    int error = ecs_new(state);
    if (error) {
        logger_log(LOGGER_LOG_LEVEL_FATAL, "failed to create ecs", error);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
