#include "core/ecs/ecs.h"
#include "core/ecs/world.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include "core/util/table.h"
#include <stddef.h>
#include <stdio.h>

#define ECS_WORLDS_START_CAPACITY 2

int ecs_init(struct ecs *out) {
    if (!out)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    status = table_init(&out->worlds, ECS_WORLDS_START_CAPACITY,
                        GLOBALS_STR_ID_MAX, sizeof(struct ecs_world));
    if (status)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    ecs_destroy(out);
    return status;
}

void ecs_destroy(struct ecs *in) {
    if (!in)
        return;

    table_destroy(&in->worlds);
}

int ecs_add_world(struct ecs *in, const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char world_id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(world_id, GLOBALS_STR_ID_MAX, "%s", id);

    struct ecs_world new_world = {0};
    int status = CORE_SUCCESS;
    status = ecs_world_init(&new_world);
    if (status)
        return status;

    status = table_insert(&in->worlds, world_id, &new_world);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_get_world(struct ecs_world **out, struct ecs *in, const char *id) {
    if (!out || !in || !id)
        return CORE_NULLPTR;

    char world_id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(world_id, GLOBALS_STR_ID_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_find((void *)out, &in->worlds, world_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_remove_world(struct ecs *in, const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char world_id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(world_id, GLOBALS_STR_ID_MAX, "%s", id);

    struct ecs_world *found = NULL;
    int status = CORE_SUCCESS;
    status = table_find((void **)&found, &in->worlds, world_id);
    if (status)
        return status;

    ecs_world_destroy(found);
    status = table_remove(NULL, &in->worlds, world_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_update_all_worlds(struct ecs *in) {
    if (!in)
        return CORE_NULLPTR;

    struct table_iterator iter;
    if (table_iterator_init(&iter, &in->worlds) == CORE_SUCCESS) {
        while (table_iterator_next(&iter)) {
            int status = CORE_SUCCESS;
            status = ecs_world_update(iter.value);
            if (status) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, status,
                                 "Updating ecs world %s failed", iter.key);
                return status;
            }
        }
    }

    return CORE_SUCCESS;
}
