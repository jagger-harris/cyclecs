#include "core/ecs/ecs.h"
#include "core/app/app.h"
#include "core/app/window.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include "core/util/mem.h"
#include "core/util/table.h"
#include <stdarg.h>

#define ECS_WORLD_START_CAPACITY 4
#define ECS_WORLD_ENTITY_START_CAPACITY 32

struct ecs {
    struct table *worlds;
};

struct ecs_world_system {
    struct ecs_world_query query;
    ecs_world_system_fn system;
};

struct ecs_world {
    struct array *entities;
    struct array *free_entities;
    struct array *pending_deletions;
    struct table *components;
    struct table *systems;
    u32 next_entity_id;
    int priority;
    float tick_rate;
    float tick_amount;
    bool should_update;
};

struct ecs_world_sparse_set {
    struct array *sparse;
    struct array *dense;
    struct array *data;
    size_t component_size;
};

static void ecs_world_destroy(struct ecs_world *in) {
    if (!in)
        return;

    struct table_iterator *comp_iter = NULL;
    int error = table_iterator_create(&comp_iter, in->components);
    if (error)
        return;

    bool comp_iter_next = false;
    while (table_iterator_next(&comp_iter_next, comp_iter) == CORE_SUCCESS &&
           comp_iter_next) {
        struct ecs_world_sparse_set *set = NULL;
        error = table_iterator_value_get((void **)&set, comp_iter);
        if (error)
            continue;

        if (set) {
            array_destroy(set->sparse);
            array_destroy(set->dense);
            array_destroy(set->data);
        }
    }

    table_iterator_destroy(comp_iter);

    struct table_iterator *sys_iter = NULL;
    error = table_iterator_create(&sys_iter, in->systems);
    if (error)
        return;

    bool sys_iter_next = false;
    while (table_iterator_next(&sys_iter_next, sys_iter) == CORE_SUCCESS &&
           sys_iter_next) {
        struct ecs_world_system *system = NULL;
        error = table_iterator_value_get((void **)&system, sys_iter);
        if (error)
            continue;

        if (system)
            ecs_world_query_destroy(&system->query);
    }

    table_iterator_destroy(sys_iter);
    array_destroy(in->entities);
    array_destroy(in->free_entities);
    array_destroy(in->pending_deletions);
    table_destroy(in->components);
    table_destroy(in->systems);
}

static int ecs_world_init(struct ecs_world *out, float tick_rate, int priority,
                          bool should_update) {
    if (!out)
        return CORE_NULLPTR;

    *out = (struct ecs_world){.next_entity_id = 0,
                              .tick_rate = tick_rate,
                              .priority = priority,
                              .should_update = should_update};

    int error =
        array_create(&out->entities, ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_create(&out->free_entities, ECS_WORLD_START_CAPACITY,
                         sizeof(u32));
    if (error)
        goto cleanup;

    error = array_create(&out->pending_deletions, ECS_WORLD_START_CAPACITY,
                         sizeof(u32));
    if (error)
        goto cleanup;

    error = table_create(&out->components, ECS_WORLD_START_CAPACITY,
                         sizeof(u32), sizeof(struct ecs_world_sparse_set));
    if (error)
        goto cleanup;

    error = table_create(&out->systems, ECS_WORLD_START_CAPACITY, sizeof(u32),
                         sizeof(struct ecs_world_system));
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    ecs_world_destroy(out);
    return error;
}

int ecs_create(struct ecs **out, struct mem *mem) {
    if (!out)
        return CORE_NULLPTR;

    struct ecs *ecs = NULL;
    int error =
        mem_alloc((void **)&ecs, mem, sizeof(struct ecs), alignof(struct ecs));
    if (error)
        return error;

    error = table_create(&ecs->worlds, ECS_WORLD_START_CAPACITY, sizeof(u32),
                         sizeof(struct ecs_world));
    if (error)
        goto cleanup;

    *out = ecs;
    return CORE_SUCCESS;

cleanup:
    ecs_destroy(ecs);
    return error;
}

void ecs_destroy(struct ecs *in) {
    if (!in)
        return;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->worlds);
    if (error)
        return;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CORE_SUCCESS && iter_next) {
        struct ecs_world *world = NULL;
        error = table_iterator_value_get((void **)&world, iter);
        if (error)
            continue;

        ecs_world_destroy(world);
    }

    table_iterator_destroy(iter);
    table_destroy(in->worlds);
}

int ecs_world_add(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world world = {0};
    int error = ecs_world_init(&world, tick_rate, priority, should_update);
    if (error)
        return error;

    return table_insert(in->worlds, &world_id, &world);
}

int ecs_world_remove(struct ecs *in, u32 world_id) {
    if (!in || !world_id)
        return CORE_NULLPTR;

    struct ecs_world *found = NULL;
    int error = table_find((void **)&found, in->worlds, &world_id);
    if (error)
        return error;

    ecs_world_destroy(found);
    return table_remove(NULL, in->worlds, &world_id);
}

int ecs_world_get(struct ecs_world **out, struct ecs *in, u32 world_id) {
    if (!out || !in)
        return CORE_NULLPTR;

    return table_find((void **)out, in->worlds, &world_id);
}

int ecs_get_all_worlds(struct table **out, struct ecs *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->worlds;
    return CORE_SUCCESS;
}

int ecs_update_all_worlds(struct ecs *in, struct app *app) {
    if (!in)
        return CORE_NULLPTR;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CORE_SUCCESS && iter_next) {
        struct ecs_world *world = NULL;
        error = table_iterator_value_get((void **)&world, iter);
        if (error)
            continue;

        u32 *key = NULL;
        error = table_iterator_key_get((void **)&key, iter);
        if (error)
            return error;

        error = ecs_world_update(world, app);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                             "Updating ecs world failed (%u)", key);
            continue;
        }
    }

    table_iterator_destroy(iter);
    return CORE_SUCCESS;
}

int ecs_iter_all_worlds(struct ecs *in, ecs_world_iter_callback callback,
                        void *data) {
    if (!in || !callback)
        return CORE_NULLPTR;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CORE_SUCCESS && iter_next) {
        struct ecs_world *world = NULL;
        error = table_iterator_value_get((void **)&world, iter);
        if (error)
            continue;

        if (!world)
            continue;

        error = callback(world, data);
        if (error)
            continue;
    }

    table_iterator_destroy(iter);
    return CORE_SUCCESS;
}

int ecs_world_entity_add(u32 *out, struct ecs_world *in) {
    if (!in || !out)
        return CORE_NULLPTR;

    size_t free_entities_length = 0;
    int error = array_length_get(&free_entities_length, in->free_entities);
    if (error)
        return error;

    u32 new_entity = U32_MAX;
    if (free_entities_length > 0) {
        error = array_elem_get_cpy(&new_entity, in->free_entities,
                                   free_entities_length - 1);
        if (error)
            return error;

        error = array_pop(in->free_entities);
        if (error)
            return error;
    } else {
        new_entity = in->next_entity_id;
        in->next_entity_id++;
    }

    error = array_push(&in->entities, &new_entity);
    if (error)
        return error;

    *out = new_entity;
    return CORE_SUCCESS;
}

static int ecs_world_entity_remove_component(struct ecs_world *in, u32 entity,
                                             u32 component_id) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world_sparse_set *set = NULL;
    int error = table_find((void **)&set, in->components, &component_id);
    if (error)
        return error;

    size_t dense_length = 0;
    error = array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_length == 0)
        return CORE_SUCCESS;

    u32 dense_index = 0;
    error = array_elem_get_cpy(&dense_index, set->sparse, entity);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CORE_INVALID_ARG;

    u32 check_entity = 0;
    error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != entity)
        return CORE_INVALID_ARG;

    size_t last_index = dense_length - 1;
    if (dense_index != last_index) {
        u32 last_entity = 0;
        void *last_data = NULL;

        error = array_elem_get_cpy(&last_entity, set->dense, last_index);
        if (error)
            return error;

        error = array_elem_get_mut(&last_data, set->data, last_index);
        if (error)
            return error;

        void *current_data = NULL;
        error = array_elem_get_mut(&current_data, set->data, dense_index);
        if (error)
            return error;

        error = array_elem_set(set->dense, dense_index, &last_entity);
        if (error)
            return error;

        memcpy(current_data, last_data, set->component_size);

        error = array_elem_set(set->sparse, last_entity, &dense_index);
        if (error)
            return error;
    }

    error = array_pop(set->dense);
    if (error)
        return error;

    error = array_pop(set->data);
    if (error)
        return error;

    u32 invalid_index = U32_MAX;
    return array_elem_set(set->sparse, entity, &invalid_index);
}

int ecs_world_entity_remove(struct ecs_world *in, u32 entity) {
    if (!in)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    size_t pending_deletions_length = 0;
    int error =
        array_length_get(&pending_deletions_length, in->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
        u32 pending_entity = U32_MAX;
        error = array_elem_get_cpy(&pending_entity, in->pending_deletions, i);
        if (error)
            continue;

        if (pending_entity == entity)
            return CORE_SUCCESS;
    }

    return array_push(&in->pending_deletions, &entity);
}

int ecs_world_component_type_add(struct ecs_world *in, u32 component_id,
                                 size_t component_size) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world_sparse_set set = {.component_size = component_size};
    int error =
        array_create(&set.sparse, ECS_WORLD_ENTITY_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error =
        array_create(&set.dense, ECS_WORLD_ENTITY_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_create(&set.data, ECS_WORLD_ENTITY_START_CAPACITY,
                         component_size);
    if (error)
        goto cleanup;

    error = table_insert(in->components, &component_id, &set);
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    array_destroy(set.sparse);
    array_destroy(set.dense);
    array_destroy(set.data);
    return error;
}

int ecs_world_component_type_remove(struct ecs_world *in, u32 component_id) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world_sparse_set *set = NULL;
    int error = table_find((void **)&set, in->components, &component_id);
    if (error)
        return error;

    array_destroy(set->sparse);
    array_destroy(set->dense);
    array_destroy(set->data);
    return table_remove(NULL, in->components, &component_id);
}

int ecs_world_component_add(struct ecs_world *in, u32 entity, u32 component_id,
                            void *data) {
    if (!in || !data)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (entity >= entities_length)
        return CORE_INVALID_ARG;

    struct ecs_world_sparse_set *set = NULL;
    error = table_find((void **)&set, in->components, &component_id);
    if (error)
        return error;

    if (!set) {
        LOGGER_LOG(LOGGER_ERROR,
                   "Adding component failed because (%s) component type does "
                   "not exist in world",
                   component_id);
        return CORE_NULLPTR;
    }

    size_t sparse_length = 0;
    error = array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    while (sparse_length <= entity) {
        u32 invalid_index = U32_MAX;
        error = array_push(&set->sparse, &invalid_index);
        if (error)
            return error;

        error = array_length_get(&sparse_length, set->sparse);
        if (error)
            return error;
    }

    size_t dense_length = 0;
    error = array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    error = array_elem_set(set->sparse, (size_t)entity, &dense_length);
    if (error)
        return error;

    error = array_push(&set->dense, &entity);
    if (error)
        return error;

    return array_push(&set->data, data);
}

int ecs_world_component_remove(struct ecs_world *in, u32 entity,
                               u32 component_id) {
    if (!in)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (entity > entities_length)
        return CORE_INVALID_ARG;

    return ecs_world_entity_remove_component(in, entity, component_id);
}

static int ecs_world_query_init_va_list(struct ecs_world_query *out,
                                        struct ecs_world *world, size_t count,
                                        va_list args) {
    if (!out || !world || count == 0)
        return CORE_INVALID_ARG;

    *out = (struct ecs_world_query){
        .world = world, .count = count, .min_set = NULL, .current_index = 0};

    int error =
        array_create(&out->sets, count, sizeof(struct ecs_world_sparse_set *));
    if (error)
        goto cleanup;

    error = array_create(&out->component_ids, count, sizeof(u32));
    if (error)
        goto cleanup;

    for (size_t i = 0; i < count; ++i) {
        u32 component_id = va_arg(args, u32);

        error = array_push(&out->component_ids, &component_id);
        if (error)
            goto cleanup;

        struct ecs_world_sparse_set *set = NULL;
        error = table_find((void **)&set, world->components, &component_id);
        if (error)
            goto cleanup;

        error = array_push(&out->sets, (void *)&set);
        if (error)
            goto cleanup;
    }

    struct ecs_world_sparse_set *first_set = NULL;
    error = array_elem_get_cpy((void *)&first_set, out->sets, 0);
    if (error)
        goto cleanup;

    out->min_set = first_set;

    size_t min_dense_length = 0;
    error = array_length_get(&min_dense_length, out->min_set->dense);
    if (error)
        return error;

    for (size_t i = 1; i < count; ++i) {
        struct ecs_world_sparse_set *current_set = NULL;
        error = array_elem_get_cpy((void *)&current_set, out->sets, i);
        if (error)
            continue;

        size_t dense_length = 0;
        error = array_length_get(&dense_length, current_set->dense);
        if (error)
            return error;

        if (dense_length < min_dense_length)
            out->min_set = current_set;
    }

    return CORE_SUCCESS;

cleanup:
    ecs_world_query_destroy(out);
    return error;
}

int ecs_world_query_init(struct ecs_world_query *out, struct ecs_world *world,
                         size_t count, ...) {
    va_list args;
    va_start(args, count);
    int error = ecs_world_query_init_va_list(out, world, count, args);
    va_end(args);
    return error;
}

void ecs_world_query_destroy(struct ecs_world_query *query) {
    if (!query)
        return;

    array_destroy(query->sets);
    array_destroy(query->component_ids);
}

int ecs_world_query_next(u32 *out, struct ecs_world_query *query) {
    if (!out || !query || !query->min_set)
        return CORE_INVALID_ARG;

    *out = U32_MAX;

    size_t min_dense_length = 0;
    int error = array_length_get(&min_dense_length, query->min_set->dense);
    if (error)
        return error;

    while (query->current_index < min_dense_length) {
        u32 entity = 0;
        error = array_elem_get_cpy(&entity, query->min_set->dense,
                                   query->current_index++);
        if (error != CORE_SUCCESS)
            return error;

        bool match = true;
        for (size_t i = 0; i < query->count; ++i) {
            struct ecs_world_sparse_set *set = NULL;
            error = array_elem_get_cpy((void *)&set, query->sets, i);
            if (error != CORE_SUCCESS) {
                match = false;
                break;
            }

            if (set == query->min_set)
                continue;

            size_t sparse_length = 0;
            error = array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (entity >= sparse_length) {
                match = false;
                break;
            }

            size_t dense_length = 0;
            error = array_length_get(&dense_length, set->dense);
            if (error)
                return error;

            u32 dense_index = 0;
            error = array_elem_get_cpy(&dense_index, set->sparse, entity);
            if (error != CORE_SUCCESS || dense_index >= dense_length) {
                match = false;
                break;
            }

            u32 check = 0;
            error = array_elem_get_cpy(&check, set->dense, dense_index);
            if (error != CORE_SUCCESS || check != entity) {
                match = false;
                break;
            }
        }

        if (match) {
            *out = entity;
            return CORE_SUCCESS;
        }
    }

    return CORE_SUCCESS;
}

int ecs_world_query_get(void **out, const struct ecs_world_query *query,
                        u32 component_id, u32 entity) {
    if (!out || !query)
        return CORE_INVALID_ARG;

    *out = NULL;

    for (size_t i = 0; i < query->count; ++i) {
        u32 stored_id = 0;
        int error = array_elem_get_cpy(&stored_id, query->component_ids, i);
        if (error)
            continue;

        if (component_id == stored_id) {
            struct ecs_world_sparse_set *set = NULL;
            error = array_elem_get_cpy((void *)&set, query->sets, i);
            if (error)
                return error;

            size_t sparse_length = 0;
            error = array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (entity >= sparse_length)
                return CORE_INVALID_ARG;

            u32 dense_index = 0;
            error = array_elem_get_cpy(&dense_index, set->sparse, entity);
            if (error)
                return error;

            size_t data_length = 0;
            error = array_length_get(&data_length, set->data);
            if (error)
                return error;

            if (dense_index >= data_length)
                return CORE_INVALID_ARG;

            error = array_elem_get_mut(out, set->data, dense_index);
            if (error)
                return error;

            return CORE_SUCCESS;
        }
    }

    return CORE_SUCCESS;
}

int ecs_world_query_get_single(void **out, const struct ecs_world *in,
                               u32 entity, u32 component_id) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    struct ecs_world_sparse_set *set = NULL;
    int error = table_find((void **)&set, in->components, &component_id);
    if (error)
        return error;

    if (!set) {
        LOGGER_LOG_ERROR(
            LOGGER_ERROR, error,
            "ECS query failed because component type (%u) does not exist",
            component_id);
        return CORE_INVALID_ARG;
    }

    size_t sparse_length = 0;
    error = array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    if (entity >= sparse_length)
        return CORE_INVALID_ARG;

    u32 dense_index = 0;
    error = array_elem_get_cpy(&dense_index, set->sparse, entity);
    if (error)
        return error;

    size_t dense_length = 0;
    error = array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CORE_INVALID_ARG;

    u32 check_entity = 0;
    error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != entity)
        return CORE_INVALID_ARG;

    return array_elem_get_mut(out, set->data, dense_index);
}

int ecs_world_system_add(struct ecs_world *in, u32 system_id,
                         ecs_world_system_fn system, size_t query_count, ...) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world_system new_system = {.system = system};

    if (query_count > 0) {
        va_list args;
        va_start(args, query_count);
        int error = ecs_world_query_init_va_list(&new_system.query, in,
                                                 query_count, args);
        va_end(args);

        if (error)
            return error;
    } else {
        new_system.query = (struct ecs_world_query){0};
    }

    int error = table_insert(in->systems, &system_id, &new_system);
    if (error) {
        if (query_count > 0)
            ecs_world_query_destroy(&new_system.query);
        return error;
    }

    return CORE_SUCCESS;
}

int ecs_world_system_remove(struct ecs_world *in, u32 system_id) {
    if (!in)
        return CORE_NULLPTR;

    struct ecs_world_system *system = NULL;
    int error = table_find((void **)&system, in->systems, &system_id);
    if (error)
        return error;

    if (!system)
        return CORE_NULLPTR;

    ecs_world_query_destroy(&system->query);
    return table_remove(NULL, in->systems, &system_id);
}

static int ecs_world_run_systems(struct ecs_world *in, struct app *app) {
    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->systems);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CORE_SUCCESS && iter_next) {
        struct ecs_world_system *system = NULL;
        error = table_iterator_value_get((void **)&system, iter);
        if (error)
            continue;

        if (!system || !system->system) {
            continue;
        }

        system->query.current_index = 0;

        error = system->system(&system->query, app);
        if (error)
            return error;
    }

    table_iterator_destroy(iter);
    return CORE_SUCCESS;
}

static int ecs_world_entity_remove_now(struct ecs_world *in, u32 entity) {
    if (!in)
        return CORE_NULLPTR;

    if (entity == U32_MAX)
        return CORE_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (entity >= entities_length)
        return CORE_INVALID_ARG;

    struct table_iterator *iter = NULL;
    error = table_iterator_create(&iter, in->components);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CORE_SUCCESS && iter_next) {
        struct ecs_world_sparse_set *set = NULL;
        error = table_iterator_value_get((void **)&set, iter);
        if (error)
            continue;

        if (!set)
            continue;

        size_t sparse_length = 0;
        error = array_length_get(&sparse_length, set->sparse);
        if (error)
            return error;

        if (entity >= sparse_length)
            continue;

        u32 dense_index = 0;
        error = array_elem_get_cpy(&dense_index, set->sparse, entity);
        if (error)
            continue;

        size_t dense_length = 0;
        error = array_length_get(&dense_length, set->dense);
        if (error)
            return error;

        if (dense_index >= dense_length)
            continue;

        u32 check_entity = 0;
        error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
        if (error || check_entity != entity)
            continue;

        u32 *key = NULL;
        error = table_iterator_key_get((void **)&key, iter);
        if (error)
            continue;

        error = ecs_world_entity_remove_component(in, entity, *key);
        if (error)
            return error;
    }

    table_iterator_destroy(iter);
    return array_push(&in->free_entities, &entity);
}

static int ecs_world_delete_entities(struct ecs_world *in) {
    if (!in)
        return CORE_NULLPTR;

    size_t pending_deletions_length = 0;
    int error =
        array_length_get(&pending_deletions_length, in->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
        u32 entity = U32_MAX;
        error = array_elem_get_cpy(&entity, in->pending_deletions, i);
        if (error)
            continue;

        error = ecs_world_entity_remove_now(in, entity);
        if (error)
            LOGGER_LOG(LOGGER_ERROR, "Failed to delete entity %u", entity);
    }

    array_clear(in->pending_deletions);
    return CORE_SUCCESS;
}

int ecs_world_update(struct ecs_world *in, struct app *app) {
    if (!in || !app)
        return CORE_NULLPTR;

    if (!in->should_update)
        return CORE_SUCCESS;

    if (in->tick_rate < 0.0f)
        return CORE_SUCCESS;

    if (in->tick_rate < 0.00001f) {
        int error = ecs_world_run_systems(in, app);
        if (error)
            return error;

        return ecs_world_delete_entities(in);
    }

    float dt = 0;
    int error = window_timing_dt_get(&dt, app->window);
    if (error)
        return error;

    float tick_interval = 1.0f / in->tick_rate;
    in->tick_amount += dt;

    while (in->tick_amount >= tick_interval) {
        error = ecs_world_run_systems(in, app);
        if (error)
            return error;

        error = ecs_world_delete_entities(in);
        if (error)
            return error;

        in->tick_amount -= tick_interval;
    }

    return CORE_SUCCESS;
}

int ecs_world_entities_length_get(size_t *out, struct ecs_world *in) {
    if (!out || !in)
        return CORE_NULLPTR;

    return array_length_get(out, in->entities);
}
