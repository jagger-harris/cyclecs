#include <assert.h>
#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/mem.h>
#include <cls/util/table.h>
#include <cls/util/xxhash32.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t CLS_ECS_WORLD_START_CAPACITY = 4;
static const size_t CLS_ECS_WORLD_ENTITY_START_CAPACITY = 32;

struct cls_ecs {
    struct cls_table *worlds;
};

struct cls_ecs_world_query {
    struct cls_array *sets;
    struct cls_array *comp_ids;
    struct cls_ecs_world_sparse_set *smallest_set;
    struct cls_ecs_world *world;
    size_t current_idx;
};

struct cls_ecs_world_system {
    cls_ecs_world_system_fn system;
    struct cls_ecs_world_query *query;
    float tick_rate;
    float tick_amount;
};

struct cls_ecs_singleton {
    void *data;
    size_t size;
};

struct cls_ecs_world {
    struct cls_array *entities;
    struct cls_array *free_entities;
    struct cls_array *pending_deletions;
    struct cls_array *queries;
    struct cls_table *components;
    struct cls_table *systems;
    struct cls_table *singletons;
    u32 next_entity_id;
    bool should_update;
};

struct cls_ecs_world_sparse_set {
    struct cls_array *sparse;
    struct cls_array *dense;
    struct cls_array *data;
    size_t component_size;
};

static void world_destroy(struct cls_ecs_world *world) {
    if (!world)
        return;

    struct cls_table_iterator *comp_iter = NULL;
    int error = cls_table_iterator_create(&comp_iter, world->components);
    if (error)
        return;

    bool next = false;
    while (cls_table_iterator_next(&next, comp_iter) == CLS_SUCCESS && next) {
        void *set_ptr = NULL;
        error = cls_table_iterator_value_get(&set_ptr, comp_iter);
        if (error)
            continue;

        struct cls_ecs_world_sparse_set *set = set_ptr;
        if (!set)
            continue;

        cls_array_destroy(set->sparse);
        cls_array_destroy(set->dense);
        cls_array_destroy(set->data);
    }

    cls_table_iterator_destroy(comp_iter);

    struct cls_table_iterator *sys_iter = NULL;
    error = cls_table_iterator_create(&sys_iter, world->systems);
    if (error)
        return;

    next = false;
    while (cls_table_iterator_next(&next, sys_iter) == CLS_SUCCESS && next) {
        void *system_ptr = NULL;
        error = cls_table_iterator_value_get(&system_ptr, sys_iter);
        if (error)
            continue;

        struct cls_ecs_world_system *system = system_ptr;
        if (system)
            cls_ecs_world_query_destroy(system->query);
    }

    cls_table_iterator_destroy(sys_iter);

    struct cls_table_iterator *singleton_iter = NULL;
    error = cls_table_iterator_create(&singleton_iter, world->singletons);
    if (error)
        return;

    next = false;
    while (cls_table_iterator_next(&next, singleton_iter) == CLS_SUCCESS &&
           next) {
        void *slot = NULL;
        error = cls_table_iterator_value_get(&slot, singleton_iter);
        if (error)
            continue;

        if (slot)
            free(((struct cls_ecs_singleton *)slot)->data);
    }

    cls_table_iterator_destroy(singleton_iter);

    cls_array_destroy(world->entities);
    cls_array_destroy(world->free_entities);
    cls_array_destroy(world->pending_deletions);
    cls_array_destroy(world->queries);
    cls_table_destroy(world->components);
    cls_table_destroy(world->systems);
    cls_table_destroy(world->singletons);
}

static int add_default_component_types(struct cls_ecs_world *world) {
    assert(world && "world is NULL");

    int error = cls_ecs_world_component_type_add(world, CLS_COMP_CAMERA,
                                                 sizeof(struct camera));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_CAMERA_ACTIVE,
                                             sizeof(struct camera_active));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_GROUP,
                                             sizeof(struct group));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_TRANSFORM,
                                             sizeof(struct transform));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_RENDERABLE,
                                             sizeof(struct renderable));
    if (error)
        return error;

    error =
        cls_ecs_world_component_type_add(world, CLS_COMP_UI, sizeof(struct ui));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_BUTTON,
                                             sizeof(struct button));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_BUTTON_GROUP,
                                             sizeof(struct button_group));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_LABEL,
                                             sizeof(struct label));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_PROGRESS_BAR,
                                             sizeof(struct progress_bar));
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int world_init(struct cls_ecs_world *world, bool should_update) {
    assert(world && "world is NULL");

    *world = (struct cls_ecs_world){.next_entity_id = 0,
                                    .should_update = should_update};

    int error = cls_array_create(&world->entities, CLS_ECS_WORLD_START_CAPACITY,
                                 sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&world->free_entities,
                             CLS_ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&world->pending_deletions,
                             CLS_ECS_WORLD_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&world->queries, CLS_ECS_WORLD_START_CAPACITY,
                             sizeof(struct cls_ecs_world_query));
    if (error)
        goto cleanup;

    error =
        cls_table_create(&world->components, CLS_ECS_WORLD_START_CAPACITY,
                         sizeof(u32), sizeof(struct cls_ecs_world_sparse_set));
    if (error)
        goto cleanup;

    error = cls_table_create(&world->systems, CLS_ECS_WORLD_START_CAPACITY,
                             sizeof(u32), sizeof(struct cls_ecs_world_system));
    if (error)
        goto cleanup;

    error = cls_table_create(&world->singletons, CLS_ECS_WORLD_START_CAPACITY,
                             sizeof(u32), sizeof(struct cls_ecs_singleton));
    if (error)
        goto cleanup;

    error = add_default_component_types(world);
    if (error)
        goto cleanup;

    return CLS_SUCCESS;

cleanup:
    world_destroy(world);
    return error;
}

static int world_component_add_by_id(struct cls_ecs_world *world, cls_entity e,
                                     u32 id, const void *comp) {
    assert(world && comp && "world or comp is NULL");

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_len = 0;
    int error = cls_array_length_get(&entities_len, world->entities);
    if (error)
        return error;

    if (e >= entities_len)
        return CLS_INVALID_ARG;

    void *set_ptr = NULL;
    error = cls_table_find(&set_ptr, world->components, &id);
    if (error)
        return error;

    struct cls_ecs_world_sparse_set *set = set_ptr;
    if (!set)
        return CLS_INVALID_ARG;

    size_t sparse_len = 0;
    error = cls_array_length_get(&sparse_len, set->sparse);
    if (error)
        return error;

    while (sparse_len <= e) {
        cls_entity invalid_e = CLS_ENTITY_MAX;
        error = cls_array_push(&set->sparse, &invalid_e);
        if (error)
            return error;

        error = cls_array_length_get(&sparse_len, set->sparse);
        if (error)
            return error;
    }

    size_t dense_len = 0;
    error = cls_array_length_get(&dense_len, set->dense);
    if (error)
        return error;

    error = cls_array_elem_set(set->sparse, (size_t)e, &dense_len);
    if (error)
        return error;

    error = cls_array_push(&set->dense, &e);
    if (error)
        return error;

    return cls_array_push(&set->data, comp);
}

static int world_entity_remove_component_by_id(struct cls_ecs_world *world,
                                               cls_entity e, u32 id) {
    assert(world && "world is NULL");

    void *set_ptr = NULL;
    int error = cls_table_find(&set_ptr, world->components, &id);
    if (error)
        return error;

    const struct cls_ecs_world_sparse_set *set = set_ptr;

    size_t dense_len = 0;
    error = cls_array_length_get(&dense_len, set->dense);
    if (error)
        return error;

    if (dense_len == 0)
        return CLS_SUCCESS;

    u32 dense_idx = 0;
    error = cls_array_elem_get_cpy(&dense_idx, set->sparse, e);
    if (error)
        return error;

    if (dense_idx >= dense_len)
        return CLS_INVALID_ARG;

    u32 check_entity = CLS_ENTITY_MAX;
    error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_idx);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    size_t last_idx = dense_len - 1;
    if (dense_idx != last_idx) {
        cls_entity last_entity = 0;

        error = cls_array_elem_get_cpy(&last_entity, set->dense, last_idx);
        if (error)
            return error;

        void *last_data = NULL;
        error = cls_array_elem_get(&last_data, set->data, last_idx);
        if (error)
            return error;

        void *current_data = NULL;
        error = cls_array_elem_get(&current_data, set->data, dense_idx);
        if (error)
            return error;

        error = cls_array_elem_set(set->dense, dense_idx, &last_entity);
        if (error)
            return error;

        memcpy(current_data, last_data, set->component_size);

        error = cls_array_elem_set(set->sparse, last_entity, &dense_idx);
        if (error)
            return error;
    }

    error = cls_array_pop(NULL, set->dense);
    if (error)
        return error;

    error = cls_array_pop(NULL, set->data);
    if (error)
        return error;

    cls_entity invalid_e = CLS_ENTITY_MAX;
    return cls_array_elem_set(set->sparse, e, &invalid_e);
}

static int world_run_systems(struct cls_ecs_world *world, struct cls_app *app) {
    assert(world && app && "world or app is NULL");

    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, world->systems);
    if (error)
        return error;

    float dt = 0;
    error = cls_window_timing_dt_get(&dt, app->window);
    if (error)
        return error;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *system_ptr = NULL;
        error = cls_table_iterator_value_get(&system_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world_system *system = system_ptr;
        if (!system || !system->system)
            continue;

        // Fix spiral of death
        if (system->tick_rate > 0.0f) {
            system->tick_amount += dt;

            if (system->tick_amount > 0.25f)
                system->tick_amount = 0.25f;

            float tick_interval = 1.0f / system->tick_rate;
            if (system->tick_amount < tick_interval)
                continue;

            while (system->tick_amount >= tick_interval) {
                system->tick_amount -= tick_interval;
                system->query->current_idx = 0;
                error = system->system(system->query, app);
                if (error)
                    return error;
            }
        } else {
            system->query->current_idx = 0;
            error = system->system(system->query, app);
            if (error)
                return error;
        }
    }

    cls_table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

static int world_entity_remove(struct cls_ecs_world *world, cls_entity e) {
    assert(world && "world is NULL");

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_len = 0;
    int error = cls_array_length_get(&entities_len, world->entities);
    if (error)
        return error;

    if (e >= entities_len)
        return CLS_INVALID_ARG;

    struct cls_table_iterator *iter = NULL;
    error = cls_table_iterator_create(&iter, world->components);
    if (error)
        return error;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *set_ptr = NULL;
        error = cls_table_iterator_value_get(&set_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world_sparse_set *set = set_ptr;
        if (!set)
            continue;

        size_t sparse_len = 0;
        error = cls_array_length_get(&sparse_len, set->sparse);
        if (error)
            return error;

        if (e >= sparse_len)
            continue;

        u32 dense_idx = 0;
        error = cls_array_elem_get_cpy(&dense_idx, set->sparse, e);
        if (error)
            continue;

        size_t dense_len = 0;
        error = cls_array_length_get(&dense_len, set->dense);
        if (error)
            return error;

        if (dense_idx >= dense_len)
            continue;

        cls_entity check_entity = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_idx);
        if (error || check_entity != e)
            continue;

        void *key_ptr = NULL;
        error = cls_table_iterator_key_get(&key_ptr, iter);
        if (error)
            continue;

        u32 *key = key_ptr;
        error = world_entity_remove_component_by_id(world, e, *key);
        if (error)
            return error;
    }

    cls_table_iterator_destroy(iter);
    return cls_array_push(&world->free_entities, &e);
}

int cls_ecs_create(struct cls_ecs **ecs, struct cls_mem *mem) {
    if (!ecs || !mem)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_mem_alloc(&instance_ptr, mem, sizeof(struct cls_ecs),
                              alignof(struct cls_ecs));
    if (error)
        return error;

    struct cls_ecs *instance = instance_ptr;
    if (!instance)
        return CLS_NULLPTR;

    error = cls_table_create(&instance->worlds, CLS_ECS_WORLD_START_CAPACITY,
                             sizeof(u32), sizeof(struct cls_ecs_world));
    if (error)
        goto cleanup;

    *ecs = instance;
    return CLS_SUCCESS;

cleanup:
    cls_ecs_destroy(instance);
    return error;
}

void cls_ecs_destroy(struct cls_ecs *ecs) {
    if (!ecs)
        return;

    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, ecs->worlds);
    if (error)
        return;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *world_ptr = NULL;
        error = cls_table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world *world = world_ptr;
        world_destroy(world);
    }

    cls_table_iterator_destroy(iter);
    cls_table_destroy(ecs->worlds);
}

int cls_ecs_world_add(struct cls_ecs_world **world, struct cls_ecs *ecs,
                      const char *id, bool should_update) {
    if (!ecs || !id)
        return CLS_NULLPTR;

    struct cls_ecs_world w = {0};
    int error = world_init(&w, should_update);
    if (error)
        return error;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    error = cls_table_insert(ecs->worlds, &hash, &w);
    if (error)
        return error;

    void *find_ptr = NULL;
    error = cls_table_find(&find_ptr, ecs->worlds, &hash);
    if (error)
        return error;

    if (world)
        *world = find_ptr;

    return CLS_SUCCESS;
}

int cls_ecs_world_remove(struct cls_ecs *ecs, const char *id) {
    if (!ecs || !id)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *world_ptr = NULL;
    error = cls_table_find(&world_ptr, ecs->worlds, &hash);
    if (error)
        return error;

    struct cls_ecs_world *world = world_ptr;
    world_destroy(world);
    return cls_table_remove(NULL, ecs->worlds, &hash);
}

int cls_ecs_world_get(struct cls_ecs_world **world, const struct cls_ecs *ecs,
                      const char *id) {
    if (!world || !ecs || !id)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *world_ptr = NULL;
    error = cls_table_find(&world_ptr, ecs->worlds, &hash);
    if (error)
        return error;

    *world = world_ptr;
    return CLS_SUCCESS;
}

int cls_ecs_world_update_all(struct cls_ecs *ecs, struct cls_app *app) {
    if (!ecs)
        return CLS_NULLPTR;

    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, ecs->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *world_ptr = NULL;
        error = cls_table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world *world = world_ptr;
        if (!world)
            continue;

        void *key_ptr = NULL;
        error = cls_table_iterator_key_get(&key_ptr, iter);
        if (error)
            continue;

        int *key = key_ptr;
        error = cls_ecs_world_update(world, app);
        if (error) {
            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error,
                                 "Updating ecs world failed (%i)", *key);
            continue;
        }
    }

    cls_table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

int cls_ecs_world_iter_all(struct cls_ecs *in, cls_ecs_world_iter_fn fn,
                           void *user_data) {
    if (!in || !fn)
        return CLS_NULLPTR;

    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *world_ptr = NULL;
        error = cls_table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct cls_ecs_world *world = world_ptr;
        if (!world)
            continue;

        error = fn(world, user_data);
        if (error)
            continue;
    }

    cls_table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

int cls_ecs_world_flush(struct cls_ecs_world *world) {
    if (!world)
        return CLS_NULLPTR;

    size_t pending_deletions_len = 0;
    int error =
        cls_array_length_get(&pending_deletions_len, world->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_len; ++i) {
        cls_entity e = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&e, world->pending_deletions, i);
        if (error)
            continue;

        error = world_entity_remove(world, e);
        if (error)
            CLS_LOGGER_LOG(CLS_LOGGER_ERROR, "Failed to delete cls_entity %u",
                           e);
    }

    cls_array_clear(world->pending_deletions);
    return CLS_SUCCESS;
}

int cls_ecs_world_entity_add(cls_entity *e, struct cls_ecs_world *world) {
    if (!e || !world)
        return CLS_NULLPTR;

    size_t free_entities_len = 0;
    int error = cls_array_length_get(&free_entities_len, world->free_entities);
    if (error)
        return error;

    cls_entity instance = CLS_ENTITY_MAX;
    if (free_entities_len > 0) {
        error = cls_array_elem_get_cpy(&instance, world->free_entities,
                                       free_entities_len - 1);
        if (error)
            return error;
        error = cls_array_pop(NULL, world->free_entities);
        if (error)
            return error;
    } else {
        instance = world->next_entity_id;
        world->next_entity_id++;
        error = cls_array_push(&world->entities, &instance);
        if (error)
            return error;
    }

    *e = instance;
    return CLS_SUCCESS;
}

int cls_ecs_world_entity_remove(struct cls_ecs_world *world, cls_entity e) {
    if (!world)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t pending_deletions_len = 0;
    int error =
        cls_array_length_get(&pending_deletions_len, world->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_len; ++i) {
        cls_entity pending = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&pending, world->pending_deletions, i);
        if (error)
            continue;

        if (pending == e)
            return CLS_SUCCESS;
    }

    return cls_array_push(&world->pending_deletions, &e);
}

int cls_ecs_world_component_type_add(struct cls_ecs_world *world,
                                     const char *id, size_t comp_size) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_ecs_world_sparse_set set = {.component_size = comp_size,
                                           .sparse = NULL,
                                           .dense = NULL,
                                           .data = NULL};
    int error = cls_array_create(
        &set.sparse, CLS_ECS_WORLD_ENTITY_START_CAPACITY, sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&set.dense, CLS_ECS_WORLD_ENTITY_START_CAPACITY,
                             sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&set.data, CLS_ECS_WORLD_ENTITY_START_CAPACITY,
                             comp_size);
    if (error)
        goto cleanup;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        goto cleanup;

    error = cls_table_insert(world->components, &hash, &set);
    if (error)
        goto cleanup;

    return CLS_SUCCESS;

cleanup:
    cls_array_destroy(set.sparse);
    cls_array_destroy(set.dense);
    cls_array_destroy(set.data);
    return error;
}

int cls_ecs_world_component_type_remove(struct cls_ecs_world *world,
                                        const char *id) {
    if (!world)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *set_ptr = NULL;
    error = cls_table_find(&set_ptr, world->components, &hash);
    if (error)
        return error;

    const struct cls_ecs_world_sparse_set *set = set_ptr;
    cls_array_destroy(set->sparse);
    cls_array_destroy(set->dense);
    cls_array_destroy(set->data);
    return cls_table_remove(NULL, world->components, &hash);
}

int cls_ecs_world_component_add(struct cls_ecs_world *world, cls_entity e,
                                const char *id, const void *comp) {
    if (!world || !id || !comp)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    return world_component_add_by_id(world, e, hash, comp);
}

int cls_ecs_world_component_remove(struct cls_ecs_world *world, cls_entity e,
                                   const char *id) {
    if (!world || !id)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_len = 0;
    int error = cls_array_length_get(&entities_len, world->entities);
    if (error)
        return error;

    if (e >= entities_len)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    return world_entity_remove_component_by_id(world, e, hash);
}

int cls_ecs_world_component_get(void **comp, const struct cls_ecs_world *world,
                                cls_entity e, const char *id) {
    if (!comp || !world)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *set_ptr = NULL;
    error = cls_table_find(&set_ptr, world->components, &hash);
    if (error)
        return error;

    const struct cls_ecs_world_sparse_set *set = set_ptr;
    if (!set) {
        CLS_LOGGER_LOG_ERROR(
            CLS_LOGGER_ERROR, error,
            "ECS query failed because component type (%s) does not exist", id);
        return CLS_INVALID_ARG;
    }

    size_t sparse_len = 0;
    error = cls_array_length_get(&sparse_len, set->sparse);
    if (error)
        return error;

    if (e >= sparse_len)
        return CLS_INVALID_ARG;

    u32 dense_idx = 0;
    error = cls_array_elem_get_cpy(&dense_idx, set->sparse, e);
    if (error)
        return error;

    size_t dense_len = 0;
    error = cls_array_length_get(&dense_len, set->dense);
    if (error)
        return error;

    if (dense_idx >= dense_len)
        return CLS_INVALID_ARG;

    cls_entity check_entity = CLS_ENTITY_MAX;
    error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_idx);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    return cls_array_elem_get(comp, set->data, dense_idx);
}

int cls_ecs_world_query_create(struct cls_ecs_world_query **query,
                               struct cls_ecs_world *world, size_t comp_count,
                               const char *ids[]) {
    if (!query || !world || !ids)
        return CLS_NULLPTR;

    if (comp_count == 0)
        return CLS_INVALID_ARG;

    struct cls_ecs_world_query *instance =
        malloc(sizeof(struct cls_ecs_world_query));
    if (!instance)
        return CLS_OUT_OF_MEMORY;

    instance->world = world;
    instance->smallest_set = NULL;
    instance->current_idx = 0;

    int error = cls_array_create(&instance->comp_ids, comp_count, sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&instance->sets, comp_count,
                             sizeof(struct cls_ecs_world_sparse_set *));
    if (error)
        goto cleanup;

    for (size_t i = 0; i < comp_count; ++i) {
        const char *id = ids[i];
        u32 hash = 0;
        error = cls_xxhash32(&hash, id, strlen(id), 0);
        if (error)
            goto cleanup;

        void *set_ptr = NULL;
        error = cls_table_find(&set_ptr, world->components, &hash);
        if (error)
            goto cleanup;

        if (!set_ptr) {
            error = CLS_INVALID_ARG;
            goto cleanup;
        }

        error = cls_array_push(&instance->comp_ids, &hash);
        if (error)
            goto cleanup;

        error = cls_array_push(&instance->sets, (const void *)&set_ptr);
        if (error)
            goto cleanup;
    }

    void *set_ptr = NULL;
    error = cls_array_elem_get(&set_ptr, instance->sets, 0);
    if (error)
        goto cleanup;

    struct cls_ecs_world_sparse_set *first_set =
        *(struct cls_ecs_world_sparse_set **)set_ptr;
    if (!first_set) {
        error = CLS_INVALID_ARG;
        goto cleanup;
    }

    instance->smallest_set = first_set;

    size_t min_dense_len = 0;
    error = cls_array_length_get(&min_dense_len, instance->smallest_set->dense);
    if (error)
        goto cleanup;

    size_t set_count = 0;
    error = cls_array_length_get(&set_count, instance->sets);
    if (error)
        goto cleanup;

    for (size_t i = 1; i < set_count; ++i) {
        struct cls_ecs_world_sparse_set *current_set = NULL;
        error = cls_array_elem_get_cpy((void *)&current_set, instance->sets, i);
        if (error)
            continue;

        size_t dense_len = 0;
        error = cls_array_length_get(&dense_len, current_set->dense);
        if (error)
            continue;

        if (dense_len < min_dense_len) {
            instance->smallest_set = current_set;
            min_dense_len = dense_len;
        }
    }

    *query = instance;
    return CLS_SUCCESS;

cleanup:
    cls_ecs_world_query_destroy(instance);
    return error;
}

void cls_ecs_world_query_destroy(struct cls_ecs_world_query *query) {
    if (!query)
        return;

    cls_array_destroy(query->comp_ids);
    cls_array_destroy(query->sets);
    free(query);
}

int cls_ecs_world_query_world_get(struct cls_ecs_world **world,
                                  struct cls_ecs_world_query *query) {
    if (!world || !query)
        return CLS_NULLPTR;

    *world = query->world;
    return CLS_SUCCESS;
}

int cls_ecs_world_query_clear(struct cls_ecs_world_query *query) {
    if (!query)
        return CLS_NULLPTR;

    query->current_idx = 0;
    return CLS_SUCCESS;
}

int cls_ecs_world_query_next(cls_entity *e, void **comps,
                             struct cls_ecs_world_query *query) {
    if (!e || !query || !query->smallest_set)
        return CLS_INVALID_ARG;
    *e = CLS_ENTITY_MAX;

    size_t min_dense_len = 0;
    int error =
        cls_array_length_get(&min_dense_len, query->smallest_set->dense);
    if (error)
        return error;

    size_t len = 0;
    error = cls_array_length_get(&len, query->comp_ids);
    if (error)
        return error;

    const cls_entity *smallest_dense = NULL;
    error = cls_array_data_get((void **)&smallest_dense,
                               query->smallest_set->dense);
    if (error)
        return error;

    while (query->current_idx < min_dense_len) {
        cls_entity current_e = smallest_dense[query->current_idx++];
        bool match = true;

        for (size_t i = 0; i < len; ++i) {
            struct cls_ecs_world_sparse_set *set = NULL;
            error = cls_array_elem_get_cpy((void *)&set, query->sets, i);
            if (error)
                return error;

            size_t sparse_len = 0;
            error = cls_array_length_get(&sparse_len, set->sparse);
            if (error)
                return error;
            if ((size_t)current_e >= sparse_len) {
                match = false;
                break;
            }

            const u32 *sparse = NULL;
            error = cls_array_data_get((void **)&sparse, set->sparse);
            if (error)
                return error;

            u32 dense_idx = sparse[current_e];

            size_t dense_len = 0;
            error = cls_array_length_get(&dense_len, set->dense);
            if (error)
                return error;
            if (dense_idx >= dense_len) {
                match = false;
                break;
            }

            const u32 *dense = NULL;
            error = cls_array_data_get((void **)&dense, set->dense);
            if (error)
                return error;
            if (dense[dense_idx] != current_e) {
                match = false;
                break;
            }

            if (comps) {
                void *data_ptr = NULL;
                error = cls_array_data_get(&data_ptr, set->data);
                if (error)
                    return error;

                comps[i] = (char *)data_ptr + dense_idx * set->component_size;
            }
        }

        if (match) {
            *e = current_e;
            return CLS_SUCCESS;
        }
    }
    return CLS_SUCCESS;
}

int cls_ecs_world_system_add(struct cls_ecs_world *world, const char *id,
                             cls_ecs_world_system_fn system, float tick_rate,
                             size_t comp_count, const char *ids[]) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_ecs_world_system new_system = {
        .system = system, .tick_rate = tick_rate, .tick_amount = 0};

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    if (comp_count > 0) {
        error = cls_ecs_world_query_create(&new_system.query, world, comp_count,
                                           ids);
        if (error)
            return error;
    } else {
        new_system.query = NULL;
    }

    error = cls_table_insert(world->systems, &hash, &new_system);
    if (error) {
        if (new_system.query)
            cls_ecs_world_query_destroy(new_system.query);

        return error;
    }

    return CLS_SUCCESS;
}

int cls_ecs_world_system_remove(struct cls_ecs_world *world, const char *id) {
    if (!world)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *system_ptr = NULL;
    error = cls_table_find(&system_ptr, world->systems, &hash);
    if (error)
        return error;

    struct cls_ecs_world_system *system = system_ptr;
    if (!system)
        return CLS_NULLPTR;

    cls_ecs_world_query_destroy(system->query);
    return cls_table_remove(NULL, world->systems, &hash);
}

int cls_ecs_world_singleton_add(struct cls_ecs_world *world, const char *id,
                                const void *data, size_t size) {
    if (!world || !id || !data)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *slot = NULL;
    if (cls_table_find(&slot, world->singletons, &hash) == CLS_SUCCESS &&
        slot) {
        struct cls_ecs_singleton *existing = slot;
        if (existing->size != size) {
            void *new_data = realloc(existing->data, size);
            if (!new_data)
                return CLS_OUT_OF_MEMORY;
            existing->data = new_data;
            existing->size = size;
        }

        memcpy(existing->data, data, size);
        return CLS_SUCCESS;
    }

    void *copy = malloc(size);
    if (!copy)
        return CLS_OUT_OF_MEMORY;

    memcpy(copy, data, size);

    struct cls_ecs_singleton singleton = {.data = copy, .size = size};
    error = cls_table_insert(world->singletons, &hash, &singleton);
    if (error) {
        free(copy);
        return error;
    }

    return CLS_SUCCESS;
}

int cls_ecs_world_singleton_remove(struct cls_ecs_world *world,
                                   const char *id) {
    if (!world || !id)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *slot = NULL;
    error = cls_table_find(&slot, world->singletons, &hash);
    if (error)
        return error;

    if (slot)
        free(((struct cls_ecs_singleton *)slot)->data);

    return cls_table_remove(NULL, world->singletons, &hash);
}

int cls_ecs_world_singleton_get(void **out, const struct cls_ecs_world *world,
                                const char *id) {
    if (!out || !world || !id)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    void *slot = NULL;
    error = cls_table_find(&slot, world->singletons, &hash);
    if (error)
        return error;

    if (!slot)
        return CLS_INVALID_ARG;

    *out = ((struct cls_ecs_singleton *)slot)->data;
    return CLS_SUCCESS;
}

int cls_ecs_world_update(struct cls_ecs_world *world, struct cls_app *app) {
    if (!world || !app)
        return CLS_NULLPTR;

    if (!world->should_update)
        return CLS_SUCCESS;

    int error = world_run_systems(world, app);
    if (error)
        return error;

    return cls_ecs_world_flush(world);
}

int cls_ecs_world_entities_length_get(size_t *len,
                                      const struct cls_ecs_world *world) {
    if (!len || !world)
        return CLS_NULLPTR;

    return cls_array_length_get(len, world->entities);
}

int cls_ecs_world_free_entities_length_get(size_t *len,
                                           const struct cls_ecs_world *world) {
    if (!len || !world)
        return CLS_NULLPTR;

    return cls_array_length_get(len, world->free_entities);
}
