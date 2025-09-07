#include "core/ecs/world.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include <stdarg.h>
#include <stdio.h>

#define ECS_WORLD_START_CAPACITY 128

struct ecs_world_system {
    ecs_world_system_callback callback;
};

int ecs_world_init(struct ecs_world *out) {
    if (!out)
        return CORE_NULLPTR;

    out->next_entity_id = 0;
    out->should_update = true;

    int status =
        table_init(&out->components, ECS_WORLD_START_CAPACITY,
                   GLOBALS_STR_ID_MAX, sizeof(struct ecs_world_sparse_set));
    if (status)
        goto cleanup;

    status = table_init(&out->systems, ECS_WORLD_START_CAPACITY,
                        GLOBALS_STR_ID_MAX, sizeof(struct ecs_world_system));
    if (status)
        goto cleanup;

    status = array_init(&out->entities, ECS_WORLD_START_CAPACITY, sizeof(u64));
    if (status)
        goto cleanup;

    status =
        array_init(&out->free_entities, ECS_WORLD_START_CAPACITY, sizeof(u64));
    if (status)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    ecs_world_destroy(out);
    return status;
}

void ecs_world_destroy(struct ecs_world *in) {
    if (!in)
        return;

    struct table_iterator iter;
    if (table_iterator_init(&iter, &in->components) == CORE_SUCCESS) {
        while (table_iterator_next(&iter)) {
            struct ecs_world_sparse_set *set =
                (struct ecs_world_sparse_set *)iter.value;
            array_destroy(&set->sparse);
            array_destroy(&set->dense);
            array_destroy(&set->data);
        }
    }

    table_destroy(&in->components);
    table_destroy(&in->systems);
    array_destroy(&in->entities);
    array_destroy(&in->free_entities);
}

int ecs_world_entity_add(u64 *out, struct ecs_world *in) {
    if (!in || !out)
        return CORE_NULLPTR;

    u64 new_entity = UINT64_MAX;
    int status = CORE_SUCCESS;

    if (in->free_entities.length > 0) {
        status = array_get_cpy(&new_entity, &in->free_entities,
                               in->free_entities.length - 1);
        if (status)
            return status;

        status = array_pop(&in->free_entities);
        if (status)
            return status;
    } else {
        new_entity = in->next_entity_id;
        in->next_entity_id++;
    }

    status = array_push(&in->entities, &new_entity);
    if (status)
        return status;

    *out = new_entity;
    return CORE_SUCCESS;
}

static int ecs_world_entity_remove_component(struct ecs_world *in, u64 entity,
                                             const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    size_t entities_count = in->entities.length;

    if (entity > entities_count)
        return CORE_INVALID_ARGS;

    struct ecs_world_sparse_set *set = NULL;
    int status = CORE_SUCCESS;
    status = table_find((void **)&set, &in->components, id);
    if (status)
        return status;

    size_t dense_length = set->dense.length;
    size_t last_index = dense_length - 1;
    size_t dense_index = UINT64_MAX;
    size_t temp_dense = UINT64_MAX;
    void *temp_data = NULL;

    status = array_get_cpy(&temp_dense, &set->dense, last_index);
    if (status)
        return status;

    status = array_get_cpy(temp_data, &set->data, last_index);
    if (status)
        return status;

    status = array_get_cpy(&dense_index, &set->sparse, entity);
    if (status)
        return status;

    status = array_set(&set->dense, dense_index, &temp_dense);
    if (status)
        return status;

    status = array_set(&set->data, dense_index, &temp_data);
    if (status)
        return status;

    status = array_set(&set->sparse, entity, &dense_index);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_entity_remove(struct ecs_world *in, u64 entity) {
    if (!in)
        return CORE_NULLPTR;

    if (entity == UINT64_MAX)
        return CORE_INVALID_ARGS;

    int status = CORE_SUCCESS;
    status = array_push(&in->free_entities, &entity);
    if (status)
        return status;

    struct table_iterator iter;
    status = table_iterator_init(&iter, &in->components);
    if (status)
        return status;

    while (table_iterator_next(&iter)) {
        struct ecs_world_sparse_set *set =
            (struct ecs_world_sparse_set *)iter.value;

        size_t sparse_index = set->sparse.length;
        u64 dense_index = 0;
        if (entity >= sparse_index)
            continue;

        status = array_get_cpy(&dense_index, &set->sparse, entity);
        if (status)
            continue;

        size_t dense_length = set->dense.length;
        if (dense_index >= dense_length)
            continue;

        u64 check_entity = 0;
        status = array_get_cpy(&check_entity, &set->dense, dense_index);
        if (status || check_entity != entity)
            continue;

        status = ecs_world_entity_remove_component(in, entity,
                                                   (const char *)iter.key);
        if (status)
            return status;
    }

    return CORE_SUCCESS;
}

int ecs_world_component_type_add(struct ecs_world *in, const char *component_id,
                                 size_t component_size) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    struct ecs_world_sparse_set set = {0};
    int status = CORE_SUCCESS;
    status = array_init(&set.sparse, ECS_WORLD_START_CAPACITY, sizeof(u64));
    if (status)
        goto cleanup;

    status = array_init(&set.dense, ECS_WORLD_START_CAPACITY, sizeof(u64));
    if (status)
        goto cleanup;

    status = array_init(&set.data, ECS_WORLD_START_CAPACITY, component_size);
    if (status)
        goto cleanup;

    status = table_insert(&in->components, id, &set);
    if (status)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    array_destroy(&set.sparse);
    array_destroy(&set.dense);
    array_destroy(&set.data);
    return status;
}

int ecs_world_component_type_remove(struct ecs_world *in,
                                    const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    struct ecs_world_sparse_set *set = NULL;
    int status = CORE_SUCCESS;
    status = table_find((void **)&set, &in->components, id);
    if (status)
        return status;

    array_destroy(&set->sparse);
    array_destroy(&set->dense);
    array_destroy(&set->data);

    table_remove(NULL, &in->components, id);
    return CORE_SUCCESS;
}

int ecs_world_component_add(struct ecs_world *in, u64 entity,
                            const char *component_id, void *data) {
    if (!in || !data || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    if (entity == UINT64_MAX)
        return CORE_INVALID_ARGS;

    size_t entities_count = in->entities.length;

    if (entity >= entities_count)
        return CORE_INVALID_ARGS;

    struct ecs_world_sparse_set *set = NULL;
    int status = table_find((void **)&set, &in->components, id);
    if (status)
        return status;

    while (set->sparse.length <= entity) {
        u64 invalid_index = UINT64_MAX;
        status = array_push(&set->sparse, &invalid_index);
        if (status)
            return status;
    }

    u64 dense_index = set->dense.length;
    status = array_set(&set->sparse, entity, &dense_index);
    if (status)
        return status;

    status = array_push(&set->dense, &entity);
    if (status)
        return status;

    status = array_push(&set->data, data);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_component_remove(struct ecs_world *in, u64 entity,
                               const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    if (entity == UINT64_MAX)
        return CORE_INVALID_ARGS;

    size_t entities_count = in->entities.length;

    if (entity > entities_count)
        return CORE_INVALID_ARGS;

    int status = ecs_world_entity_remove_component(in, entity, id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_query_data(void **out, const struct ecs_world *in, u64 entity,
                         const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    if (entity == UINT64_MAX)
        return CORE_INVALID_ARGS;

    struct ecs_world_sparse_set *set = NULL;
    int status = table_find((void **)&set, &in->components, id);
    if (status)
        return status;

    if (entity >= set->sparse.length)
        return CORE_INVALID_ARGS;

    u64 dense_index = 0;
    status = array_get_cpy(&dense_index, &set->sparse, entity);
    if (status)
        return status;

    if (dense_index >= set->dense.length)
        return CORE_INVALID_ARGS;

    u64 check_entity = 0;
    status = array_get_cpy(&check_entity, &set->dense, dense_index);
    if (status) {
        return status;
    }

    if (check_entity != entity) {
        return CORE_INVALID_ARGS;
    }

    status = array_get(out, &set->data, dense_index);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_query_all_data(struct array **out, const struct ecs_world *in,
                             const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    struct ecs_world_sparse_set *set = NULL;
    int status = CORE_SUCCESS;
    status = table_find((void **)&set, &in->components, id);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                         "Finding component type from ecs world query failed");
        return status;
    }

    *out = &set->data;
    return CORE_SUCCESS;
}

int ecs_world_query_all_entities(struct array **out, const struct ecs_world *in,
                                 const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", component_id);

    struct ecs_world_sparse_set *set = NULL;
    int status = CORE_SUCCESS;
    status = table_find((void **)&set, &in->components, id);
    if (status) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                         "Finding component type from ecs world query failed");
        return status;
    }

    *out = &set->dense;
    return CORE_SUCCESS;
}

int ecs_world_system_add(struct ecs_world *in, const char *system_id,
                         ecs_world_system_callback callback) {
    if (!in || !system_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", system_id);

    struct ecs_world_system new_system = {.callback = callback};
    int status = CORE_SUCCESS;
    status = table_insert(&in->systems, id, &new_system);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_system_remove(struct ecs_world *in, const char *system_id) {
    if (!in || !system_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    snprintf(id, GLOBALS_STR_ID_MAX, "%s", system_id);

    int status = table_remove(NULL, &in->systems, id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int ecs_world_update(const struct ecs_world *in) {
    if (!in)
        return CORE_NULLPTR;

    if (!in->should_update)
        return CORE_SUCCESS;

    int status = CORE_SUCCESS;
    struct table_iterator iter;
    status = table_iterator_init(&iter, &in->systems);
    if (status)
        return status;

    while (table_iterator_next(&iter)) {
        struct ecs_world_system *system = (struct ecs_world_system *)iter.value;

        if (!system || !system->callback)
            continue;

        status = system->callback(in);
        if (status)
            return status;
    }

    return CORE_SUCCESS;
}
