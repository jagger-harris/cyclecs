#include "ecs.h"
#include "../util/error.h"
#include "../util/logger.h"
#include <stddef.h>

struct entities {
    size_t capacity;
    size_t count;
    ecs_entity data[];
};

struct components {
    size_t capacity;
    size_t count;
    ecs_component data[];
};

struct systems {
    size_t capacity;
    size_t count;
    ecs_system data[];
};

typedef struct ecs {
    ecs_entity *entities;
    ecs_system *systems;
} ecs;

int ecs_new(ecs *out) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    if (error)
        goto error;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to create ecs", error);
    return error;
}

int ecs_delete(ecs *in) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_add_entity(ecs_entity *out, ecs *in) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_remove_entity(ecs *in, ecs_entity entity) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_add_component_type(ecs *in, ecs_component_type typentity) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_remove_component_type(ecs *in, ecs_component_type typentity) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_add_component(ecs *in, ecs_entity entity) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_remove_component(ecs *in, ecs_entity entity) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_remove_system(ecs *in, ecs_system system) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}

int ecs_update_systems(ecs *in) {
    int error = CORE_ERROR_SUCCESS;

    if (error)
        goto error;

    return error;

error:
    logger_log(LOGGER_LOG_LEVEL_ERROR, "failed to delete ecs", error);
    return error;
}
