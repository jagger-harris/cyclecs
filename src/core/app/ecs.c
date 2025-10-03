#include "core/app/ecs.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include "core/util/types.h"

#define ECS_START_CAPACITY 2
#define ECS_WORLD_START_CAPACITY 32

struct ecs_world_system {
    ecs_world_system_callback callback;
};

static inline int ecs_normalize_key(char *out, const char *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    memset(out, 0, GLOBALS_STR_ID_MAX);

    size_t len = strlen(in);
    if (len >= GLOBALS_STR_ID_MAX)
        return CORE_INVALID_ARG;

    memcpy(out, in, len);
    out[len] = '\0';
    return CORE_SUCCESS;
}

int ecs_init(struct ecs *out) {
    if (!out)
        return CORE_NULLPTR;

    int error = table_init(&out->worlds, ECS_START_CAPACITY, GLOBALS_STR_ID_MAX,
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

    while (table_iterator_next(&iter)) {
        struct ecs_world *world = iter.value;
        ecs_world_destroy(world);
    }

    table_destroy(&in->worlds);
}

int ecs_add_world(struct ecs *in, const char *world_id) {
    if (!in || !world_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, world_id);
    if (error)
        return error;

    struct ecs_world new_world = {0};
    error = ecs_world_init(&new_world);
    if (error)
        return error;

    error = table_insert(&in->worlds, id, &new_world);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_get_world(struct ecs_world **out, struct ecs *in,
                  const char *world_id) {
    if (!out || !in || !world_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, world_id);
    if (error)
        return error;

    error = table_find((void **)out, &in->worlds, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_remove_world(struct ecs *in, const char *world_id) {
    if (!in || !world_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, world_id);
    if (error)
        return error;

    struct ecs_world *found = NULL;
    error = table_find((void **)&found, &in->worlds, world_id);
    if (error)
        return error;

    ecs_world_destroy(found);
    error = table_remove(NULL, &in->worlds, id);
    if (error)
        return error;

    return CORE_SUCCESS;
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

    while (table_iterator_next(&iter)) {
        error = ecs_world_update(iter.value, app);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                             "Updating ecs world failed (%s)", iter.key);
            continue;
        }
    }

    return CORE_SUCCESS;
}

int ecs_world_init(struct ecs_world *out) {
    if (!out)
        return CORE_NULLPTR;

    out->next_entity_id = 0;
    out->should_update = true;

    int error =
        array_init(&out->entities, ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error =
        array_init(&out->free_entities, ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = table_init(&out->components, ECS_WORLD_START_CAPACITY,
                       GLOBALS_STR_ID_MAX, sizeof(struct ecs_world_sparse_set));
    if (error)
        goto cleanup;

    error = table_init(&out->systems, ECS_WORLD_START_CAPACITY,
                       GLOBALS_STR_ID_MAX, sizeof(struct ecs_world_system));
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    ecs_world_destroy(out);
    return error;
}

void ecs_world_destroy(struct ecs_world *in) {
    if (!in)
        return;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->components);
    if (error)
        return;

    while (table_iterator_next(&iter)) {
        struct ecs_world_sparse_set *set = iter.value;
        array_destroy(&set->sparse);
        array_destroy(&set->dense);
        array_destroy(&set->data);
    }

    table_destroy(&in->components);
    table_destroy(&in->systems);
    array_destroy(&in->entities);
    array_destroy(&in->free_entities);
}

int ecs_world_entity_add(u32 *out, struct ecs_world *in) {
    if (!in || !out)
        return CORE_NULLPTR;

    u32 new_entity = U32_MAX;

    if (in->free_entities.length > 0) {
        int error = array_get_cpy(&new_entity, &in->free_entities,
                                  in->free_entities.length - 1);
        if (error)
            return error;

        error = array_pop(&in->free_entities);
        if (error)
            return error;
    } else {
        new_entity = in->next_entity_id;
        in->next_entity_id++;
    }

    int error = array_push(&in->entities, &new_entity);
    if (error)
        return error;

    *out = new_entity;
    return CORE_SUCCESS;
}

static int ecs_world_entity_remove_component(struct ecs_world *in, u32 entity,
                                             const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    struct ecs_world_sparse_set *set = NULL;
    int error = table_find((void **)&set, &in->components, component_id);
    if (error)
        return error;

    size_t dense_length = set->dense.length;

    if (dense_length == 0)
        return CORE_SUCCESS;

    u32 dense_index = 0;
    error = array_get_cpy(&dense_index, &set->sparse, entity);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CORE_INVALID_ARG;

    u32 check_entity = 0;
    error = array_get_cpy(&check_entity, &set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != entity)
        return CORE_INVALID_ARG;

    size_t last_index = dense_length - 1;

    if (dense_index != last_index) {
        u32 last_entity = 0;
        void *last_data = NULL;

        error = array_get_cpy(&last_entity, &set->dense, last_index);
        if (error)
            return error;

        error = array_get(&last_data, &set->data, last_index);
        if (error)
            return error;

        void *current_data = NULL;
        error = array_get(&current_data, &set->data, dense_index);
        if (error)
            return error;

        error = array_set(&set->dense, dense_index, &last_entity);
        if (error)
            return error;

        memcpy(current_data, last_data, set->component_size);

        error = array_set(&set->sparse, last_entity, &dense_index);
        if (error)
            return error;
    }

    error = array_pop(&set->dense);
    if (error)
        return error;

    error = array_pop(&set->data);
    if (error)
        return error;

    u32 invalid_index = U32_MAX;
    error = array_set(&set->sparse, entity, &invalid_index);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_entity_remove(struct ecs_world *in, u32 entity) {
    if (!in)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    size_t entities_length = in->entities.length;

    if (entity >= entities_length)
        return CORE_INVALID_ARG;

    int error = array_push(&in->free_entities, &entity);
    if (error)
        return error;

    struct table_iterator iter;
    error = table_iterator_init(&iter, &in->components);
    if (error)
        return error;

    while (table_iterator_next(&iter)) {
        struct ecs_world_sparse_set *set = iter.value;

        if (entity >= set->sparse.length)
            continue;

        u32 dense_index = 0;
        error = array_get_cpy(&dense_index, &set->sparse, entity);
        if (error)
            continue;

        if (dense_index >= set->dense.length)
            continue;

        u32 check_entity = 0;
        error = array_get_cpy(&check_entity, &set->dense, dense_index);
        if (error || check_entity != entity)
            continue;

        error = ecs_world_entity_remove_component(in, entity,
                                                  (const char *)iter.key);
        if (error)
            return error;
    }

    return CORE_SUCCESS;
}
int ecs_world_component_type_add(struct ecs_world *in, const char *component_id,
                                 size_t component_size) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    struct ecs_world_sparse_set set = {0};
    error = array_init(&set.sparse, ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_init(&set.dense, ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_init(&set.data, ECS_WORLD_START_CAPACITY, component_size);
    if (error)
        goto cleanup;

    error = table_insert(&in->components, id, &set);
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    array_destroy(&set.sparse);
    array_destroy(&set.dense);
    array_destroy(&set.data);
    return error;
}

int ecs_world_component_type_remove(struct ecs_world *in,
                                    const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, &in->components, id);
    if (error)
        return error;

    array_destroy(&set->sparse);
    array_destroy(&set->dense);
    array_destroy(&set->data);
    table_remove(NULL, &in->components, id);
    return CORE_SUCCESS;
}

int ecs_world_component_add(struct ecs_world *in, u32 entity,
                            const char *component_id, void *data) {
    if (!in || !data || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    if (entity >= in->entities.length)
        return CORE_INVALID_ARG;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, &in->components, id);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR,
                   "Adding component failed because (%s) component type does "
                   "not exist",
                   component_id);
        return error;
    }

    while (set->sparse.length <= entity) {
        u32 invalid_index = U32_MAX;
        error = array_push(&set->sparse, &invalid_index);
        if (error)
            return error;
    }

    error = array_set(&set->sparse, (size_t)entity, &set->dense.length);
    if (error)
        return error;

    error = array_push(&set->dense, &entity);
    if (error)
        return error;

    error = array_push(&set->data, data);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_component_remove(struct ecs_world *in, u32 entity,
                               const char *component_id) {
    if (!in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    if (entity > in->entities.length)
        return CORE_INVALID_ARG;

    error = ecs_world_entity_remove_component(in, entity, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_query_data(void **out, const struct ecs_world *in, u32 entity,
                         const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, &in->components, id);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "ECS query failed (does not exist)");
        return error;
    }

    if (entity >= set->sparse.length)
        return CORE_INVALID_ARG;

    u32 dense_index = 0;
    error = array_get_cpy(&dense_index, &set->sparse, entity);
    if (error)
        return error;

    if (dense_index >= set->dense.length)
        return CORE_INVALID_ARG;

    u32 check_entity = 0;
    error = array_get_cpy(&check_entity, &set->dense, dense_index);
    if (error) {
        return error;
    }

    if (check_entity != entity) {
        return CORE_INVALID_ARG;
    }

    error = array_get(out, &set->data, dense_index);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_query_all_data(struct array **out, const struct ecs_world *in,
                             const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, &in->components, id);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Finding component type from ecs world query failed");
        return error;
    }

    *out = &set->data;
    return CORE_SUCCESS;
}

int ecs_world_query_all_entities(struct array **out, const struct ecs_world *in,
                                 const char *component_id) {
    if (!out || !in || !component_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, component_id);
    if (error)
        return error;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, &in->components, id);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Finding component type from ecs world query failed");
        return error;
    }

    *out = &set->dense;
    return CORE_SUCCESS;
}

int ecs_world_system_add(struct ecs_world *in, const char *system_id,
                         ecs_world_system_callback callback) {
    if (!in || !system_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, system_id);
    if (error)
        return error;

    struct ecs_world_system new_system = {.callback = callback};
    error = table_insert(&in->systems, id, &new_system);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_system_remove(struct ecs_world *in, const char *system_id) {
    if (!in || !system_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int error = ecs_normalize_key(id, system_id);
    if (error)
        return error;

    error = table_remove(NULL, &in->systems, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int ecs_world_update(struct ecs_world *in, struct app *app) {
    if (!in || !app)
        return CORE_NULLPTR;

    if (!in->should_update)
        return CORE_SUCCESS;

    struct table_iterator iter;
    int error = table_iterator_init(&iter, &in->systems);
    if (error)
        return error;

    while (table_iterator_next(&iter)) {
        struct ecs_world_system *system = iter.value;

        if (!system || !system->callback)
            continue;

        error = system->callback(in, app);
        if (error)
            return error;
    }

    return CORE_SUCCESS;
}
