#include "core/ecs/ecs.h"
#include "core/app/app.h"
#include "core/ecs/world.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"

#define ECS_EVENT_START_CAPACITY 32
#define ECS_WORLD_START_CAPACITY 4

int ecs_init(struct ecs *out) {
    if (!out)
        return CORE_NULLPTR;

    int error = table_init(&out->worlds, ECS_WORLD_START_CAPACITY, sizeof(u32),
                           sizeof(struct ecs_world));
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    ecs_destroy(out);
    return error;
}

void ecs_destroy(struct ecs *in) {
    if (!in)
        return;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->worlds);
    if (error)
        return;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, &iter) == CORE_SUCCESS &&
           iter_next) {
        struct ecs_world *world = iter.value;
        ecs_world_destroy(world);
    }

    table_destroy(&in->worlds);
}

int ecs_add_world(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world world = {0};
    int error = ecs_world_init(&world, tick_rate, priority, should_update);
    if (error)
        return error;

    return table_insert(&in->worlds, &world_id, &world);
}

int ecs_get_world(struct ecs_world **out, struct ecs *in, u32 world_id) {
    if (!out || !in)
        return CORE_NULLPTR;

    return table_find((void **)out, &in->worlds, &world_id);
}

int ecs_remove_world(struct ecs *in, u32 world_id) {
    if (!in || !world_id)
        return CORE_NULLPTR;

    struct ecs_world *found = NULL;
    int error = table_find((void **)&found, &in->worlds, &world_id);
    if (error)
        return error;

    ecs_world_destroy(found);
    return table_remove(NULL, &in->worlds, &world_id);
}

int ecs_get_all_worlds(struct table **out, struct ecs *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = &in->worlds;
    return CORE_SUCCESS;
}

int ecs_update_all_worlds(struct ecs *in, struct app *app) {
    if (!in)
        return CORE_NULLPTR;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, &iter) == CORE_SUCCESS &&
           iter_next) {
        PROFILER_START(ecs_world_update);
        error = ecs_world_update(iter.value, app);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                             "Updating ecs world failed (%u)", iter.key);
            continue;
        }
        PROFILER_END(ecs_world_update);
    }

    return CORE_SUCCESS;
}
