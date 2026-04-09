#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/util/allocator.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <cls/util/xxhash32.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define CLS_ECS_WORLD_START_CAPACITY 4
#define CLS_ECS_WORLD_ENTITY_START_CAPACITY 32
#define CLS_ECS_SCENE_FILE_SIG 0xE7EFD7A7 // çï×§ (cyclecs scene)
#define CLS_ECS_SCENE_FILE_VER 1

struct cls_ecs {
    struct cls_table *worlds;
};

struct cls_ecs_scene {
    const char *id;
    struct cls_array *entities;
};

struct cls_ecs_scene_entity {
    struct cls_array *components; // cls_entity can have more than one component
};

// TODO: Naive approach. Would be better to make a specific data structure for
// more efficient serialization
struct cls_ecs_comp_serialized {
    u32 id;
    size_t size;
    void *comp;
};

struct cls_ecs_world_query {
    struct cls_array *sets;
    struct cls_array *comp_ids;
    struct cls_ecs_world_sparse_set *min_set;
    struct cls_ecs_world *world;
    size_t arg_count;
    size_t current_index;
};

struct cls_ecs_world_system {
    cls_ecs_world_system_fn system;
    struct cls_ecs_world_query *query;
    void *user_data;
};

struct cls_ecs_world {
    struct cls_array *entities;
    struct cls_array *free_entities;
    struct cls_array *pending_deletions;
    struct cls_table *components;
    struct cls_table *systems;
    u32 next_entity_id;
    int priority;
    float tick_rate;
    float tick_amount;
    bool should_update;
};

struct cls_ecs_world_sparse_set {
    struct cls_array *sparse;
    struct cls_array *dense;
    struct cls_array *data;
    size_t component_size;
};

static void cls_ecs_world_destroy(struct cls_ecs_world *world) {
    if (!world)
        return;

    struct cls_table_iterator *comp_iter = NULL;
    int error = cls_table_iterator_create(&comp_iter, world->components);
    if (error)
        return;

    bool comp_iter_next = false;
    while (cls_table_iterator_next(&comp_iter_next, comp_iter) == CLS_SUCCESS &&
           comp_iter_next) {
        void *set_ptr = NULL;
        error = cls_table_iterator_value_get(&set_ptr, comp_iter);
        if (error)
            continue;

        struct cls_ecs_world_sparse_set *set = set_ptr;
        if (set) {
            cls_array_destroy(set->sparse);
            cls_array_destroy(set->dense);
            cls_array_destroy(set->data);
        }
    }

    cls_table_iterator_destroy(comp_iter);

    struct cls_table_iterator *sys_iter = NULL;
    error = cls_table_iterator_create(&sys_iter, world->systems);
    if (error)
        return;

    bool sys_iter_next = false;
    while (cls_table_iterator_next(&sys_iter_next, sys_iter) == CLS_SUCCESS &&
           sys_iter_next) {
        void *system_ptr = NULL;
        error = cls_table_iterator_value_get(&system_ptr, sys_iter);
        if (error)
            continue;

        struct cls_ecs_world_system *system = system_ptr;
        if (system) {
            if (system->user_data)
                free(system->user_data);

            cls_ecs_world_query_destroy(system->query);
        }
    }

    cls_table_iterator_destroy(sys_iter);
    cls_array_destroy(world->entities);
    cls_array_destroy(world->free_entities);
    cls_array_destroy(world->pending_deletions);
    cls_table_destroy(world->components);
    cls_table_destroy(world->systems);
}

static int add_default_component_types(struct cls_ecs_world *world) {
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

    error = cls_ecs_world_component_type_add(world, CLS_COMP_LABEL_GROUP,
                                             sizeof(struct label_group));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(world, CLS_COMP_PROGRESS_BAR,
                                             sizeof(struct progress_bar));
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int cls_ecs_world_init(struct cls_ecs_world *world, float tick_rate,
                              int priority, bool should_update) {
    if (!world)
        return CLS_NULLPTR;

    *world = (struct cls_ecs_world){.next_entity_id = 0,
                                    .tick_rate = tick_rate,
                                    .priority = priority,
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

    error =
        cls_table_create(&world->components, CLS_ECS_WORLD_START_CAPACITY,
                         sizeof(u32), sizeof(struct cls_ecs_world_sparse_set));
    if (error)
        goto cleanup;

    error = cls_table_create(&world->systems, CLS_ECS_WORLD_START_CAPACITY,
                             sizeof(u32), sizeof(struct cls_ecs_world_system));
    if (error)
        goto cleanup;

    error = add_default_component_types(world);
    if (error)
        goto cleanup;

    return CLS_SUCCESS;

cleanup:
    cls_ecs_world_destroy(world);
    return error;
}

static void cls_ecs_scene_entity_destroy(struct cls_ecs_scene_entity *e) {
    if (!e || !e->components)
        return;

    size_t comp_length = 0;
    cls_array_length_get(&comp_length, e->components);

    for (size_t i = 0; i < comp_length; ++i) {
        void *comp_ptr = NULL;
        cls_array_elem_get(&comp_ptr, e->components, i);
        struct cls_ecs_comp_serialized *comp = comp_ptr;
        if (comp && comp->comp)
            free(comp->comp);
    }

    cls_array_destroy(e->components);
}

static int cls_ecs_scene_entity_create(struct cls_ecs_scene_entity *scene_e,
                                       const struct cls_ecs_world *world,
                                       cls_entity e) {
    if (!scene_e || !world)
        return CLS_NULLPTR;

    int error = cls_array_create(&scene_e->components, 8,
                                 sizeof(struct cls_ecs_comp_serialized));
    if (error)
        return error;

    struct cls_table_iterator *iter = NULL;
    error = cls_table_iterator_create(&iter, world->components);
    if (error)
        goto cleanup;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *id_ptr = NULL;
        error = cls_table_iterator_key_get(&id_ptr, iter);
        if (error)
            continue;

        u32 *id = id_ptr;

        void *set_ptr = NULL;
        error = cls_table_iterator_value_get(&set_ptr, iter);
        if (error)
            continue;

        const struct cls_ecs_world_sparse_set *set = set_ptr;
        if (!set)
            continue;

        size_t sparse_length = 0;
        error = cls_array_length_get(&sparse_length, set->sparse);
        if (error)
            continue;

        u32 dense_index = 0;
        error = cls_array_elem_get_cpy(&dense_index, set->sparse, e);
        if (error)
            continue;

        size_t dense_length = 0;
        error = cls_array_length_get(&dense_length, set->dense);
        if (error || dense_index >= dense_length)
            continue;

        cls_entity check_entity = 0;
        error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_index);
        if (error || check_entity != e)
            continue;

        void *comp = NULL;
        error = cls_array_elem_get(&comp, set->data, dense_index);
        if (error)
            continue;

        // TODO: Use arena alloc
        void *comp_cpy = malloc(set->component_size);
        if (!comp_cpy)
            goto cleanup;

        memcpy(comp_cpy, comp, set->component_size);

        struct cls_ecs_comp_serialized saved = {
            .id = *id, .size = set->component_size, .comp = comp_cpy};

        error = cls_array_push(&scene_e->components, &saved);
        if (error)
            goto cleanup;
    }

    cls_table_iterator_destroy(iter);
    return CLS_SUCCESS;

cleanup:
    cls_ecs_scene_entity_destroy(scene_e);
    cls_table_iterator_destroy(iter);
    return error;
}

static int cls_ecs_world_component_add_by_id(struct cls_ecs_world *world,
                                             cls_entity e, u32 id,
                                             const void *comp) {
    if (!world || !comp)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = cls_array_length_get(&entities_length, world->entities);
    if (error)
        return error;

    if (e >= entities_length)
        return CLS_INVALID_ARG;

    void *set_ptr = NULL;
    error = cls_table_find(&set_ptr, world->components, &id);
    if (error)
        return error;

    struct cls_ecs_world_sparse_set *set = set_ptr;
    if (!set)
        return CLS_INVALID_ARG;

    size_t sparse_length = 0;
    error = cls_array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    while (sparse_length <= e) {
        cls_entity invalid_e = CLS_ENTITY_MAX;
        error = cls_array_push(&set->sparse, &invalid_e);
        if (error)
            return error;

        error = cls_array_length_get(&sparse_length, set->sparse);
        if (error)
            return error;
    }

    size_t dense_length = 0;
    error = cls_array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    error = cls_array_elem_set(set->sparse, (size_t)e, &dense_length);
    if (error)
        return error;

    error = cls_array_push(&set->dense, &e);
    if (error)
        return error;

    return cls_array_push(&set->data, comp);
}

static int
cls_ecs_world_entity_remove_component_by_id(struct cls_ecs_world *world,
                                            cls_entity e, u32 id) {
    if (!world || !id)
        return CLS_NULLPTR;

    void *set_ptr = NULL;
    int error = cls_table_find(&set_ptr, world->components, &id);
    if (error)
        return error;

    const struct cls_ecs_world_sparse_set *set = set_ptr;

    size_t dense_length = 0;
    error = cls_array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_length == 0)
        return CLS_SUCCESS;

    u32 dense_index = 0;
    error = cls_array_elem_get_cpy(&dense_index, set->sparse, e);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CLS_INVALID_ARG;

    u32 check_entity = CLS_ENTITY_MAX;
    error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    size_t last_index = dense_length - 1;
    if (dense_index != last_index) {
        cls_entity last_entity = 0;

        error = cls_array_elem_get_cpy(&last_entity, set->dense, last_index);
        if (error)
            return error;

        void *last_data = NULL;
        error = cls_array_elem_get(&last_data, set->data, last_index);
        if (error)
            return error;

        void *current_data = NULL;
        error = cls_array_elem_get(&current_data, set->data, dense_index);
        if (error)
            return error;

        error = cls_array_elem_set(set->dense, dense_index, &last_entity);
        if (error)
            return error;

        memcpy(current_data, last_data, set->component_size);

        error = cls_array_elem_set(set->sparse, last_entity, &dense_index);
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

static int
cls_ecs_world_query_create_from_va_list(struct cls_ecs_world_query **query,
                                        struct cls_ecs_world *world,
                                        size_t arg_count, va_list args) {
    if (!query || !world || arg_count == 0)
        return CLS_INVALID_ARG;

    struct cls_ecs_world_query *instance =
        malloc(sizeof(struct cls_ecs_world_query));
    if (!instance)
        return CLS_OUT_OF_MEMORY;

    instance->world = world;
    instance->arg_count = arg_count;
    instance->min_set = NULL;
    instance->current_index = 0;

    int error = cls_array_create(&instance->comp_ids, arg_count, sizeof(u32));
    if (error)
        goto cleanup;

    error = cls_array_create(&instance->sets, arg_count,
                             sizeof(struct cls_ecs_world_sparse_set *));
    if (error)
        goto cleanup;

    for (size_t i = 0; i < arg_count; ++i) {
        const char *id = va_arg(args, const char *);
        u32 hash = 0;
        error = cls_xxhash32(&hash, id, strlen(id), 0);
        if (error)
            return error;

        error = cls_array_push(&instance->comp_ids, &hash);
        if (error)
            continue;

        void *set_ptr = NULL;
        error = cls_table_find(&set_ptr, world->components, &hash);
        if (error)
            continue;

        if (!set_ptr)
            continue;

        error = cls_array_push(&instance->sets, (const void *)&set_ptr);
        if (error)
            continue;
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

    instance->min_set = first_set;

    size_t min_dense_length = 0;
    error = cls_array_length_get(&min_dense_length, instance->min_set->dense);
    if (error)
        goto cleanup;

    size_t set_count = 0;
    error = cls_array_length_get(&set_count, instance->sets);
    if (error)
        goto cleanup;

    for (size_t i = 1; i < arg_count; ++i) {
        struct cls_ecs_world_sparse_set *current_set = NULL;
        error = cls_array_elem_get_cpy((void *)&current_set, instance->sets, i);
        if (error)
            continue;

        size_t dense_length = 0;
        error = cls_array_length_get(&dense_length, current_set->dense);
        if (error)
            continue;

        if (dense_length < min_dense_length) {
            instance->min_set = current_set;
            min_dense_length = dense_length;
        }
    }

    *query = instance;
    return CLS_SUCCESS;

cleanup:
    cls_ecs_world_query_destroy(instance);
    return error;
}

static int cls_ecs_world_run_systems(struct cls_ecs_world *world,
                                     struct cls_app *app) {
    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, world->systems);
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

        system->query->current_index = 0;

        error = system->system(system->query, app, system->user_data);
        if (error)
            return error;
    }

    cls_table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

static int cls_ecs_world_entity_remove_now(struct cls_ecs_world *world,
                                           cls_entity e) {
    if (!world)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = cls_array_length_get(&entities_length, world->entities);
    if (error)
        return error;

    if (e >= entities_length)
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

        size_t sparse_length = 0;
        error = cls_array_length_get(&sparse_length, set->sparse);
        if (error)
            return error;

        if (e >= sparse_length)
            continue;

        u32 dense_index = 0;
        error = cls_array_elem_get_cpy(&dense_index, set->sparse, e);
        if (error)
            continue;

        size_t dense_length = 0;
        error = cls_array_length_get(&dense_length, set->dense);
        if (error)
            return error;

        if (dense_index >= dense_length)
            continue;

        cls_entity check_entity = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_index);
        if (error || check_entity != e)
            continue;

        void *key_ptr = NULL;
        error = cls_table_iterator_key_get(&key_ptr, iter);
        if (error)
            continue;

        u32 *key = key_ptr;
        error = cls_ecs_world_entity_remove_component_by_id(world, e, *key);
        if (error)
            return error;
    }

    cls_table_iterator_destroy(iter);
    return cls_array_push(&world->free_entities, &e);
}

static int cls_ecs_world_delete_entities(struct cls_ecs_world *world) {
    if (!world)
        return CLS_NULLPTR;

    size_t pending_deletions_length = 0;
    int error = cls_array_length_get(&pending_deletions_length,
                                     world->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
        cls_entity e = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&e, world->pending_deletions, i);
        if (error)
            continue;

        error = cls_ecs_world_entity_remove_now(world, e);
        if (error)
            CLS_LOGGER_LOG(CLS_LOGGER_ERROR, "Failed to delete cls_entity %u",
                           e);
    }

    cls_array_clear(world->pending_deletions);
    return CLS_SUCCESS;
}

int cls_ecs_create(struct cls_ecs **ecs, struct cls_allocator *alloc) {
    if (!ecs)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_allocator_alloc(
        &instance_ptr, alloc, sizeof(struct cls_ecs), alignof(struct cls_ecs));
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
        cls_ecs_world_destroy(world);
    }

    cls_table_iterator_destroy(iter);
    cls_table_destroy(ecs->worlds);
}

int cls_ecs_scene_create_from_world(struct cls_ecs_scene **scene,
                                    const struct cls_ecs_world *world,
                                    const char *scene_id) {
    if (!scene || !world || !scene_id)
        return CLS_NULLPTR;

    struct cls_ecs_scene *instance = malloc(sizeof(struct cls_ecs_scene));
    if (!scene)
        return CLS_OUT_OF_MEMORY;

    instance->id = scene_id;

    int error = cls_array_create(&instance->entities, 32,
                                 sizeof(struct cls_ecs_scene_entity));
    if (error)
        goto cleanup;

    size_t e_length = 0;
    error = cls_ecs_world_entities_length_get(&e_length, world);
    if (error)
        goto cleanup;

    for (size_t i = 0; i < e_length; ++i) {
        cls_entity e = 0;
        error = cls_array_elem_get_cpy(&e, world->entities, i);
        if (error)
            continue;

        struct cls_ecs_scene_entity scene_entity = {0};
        error = cls_ecs_scene_entity_create(&scene_entity, world, e);
        if (error) {
            CLS_LOGGER_LOG(
                CLS_LOGGER_WARN,
                "Saving cls_entity (%u) to scene (%s) from world failed", e,
                scene_id);
            continue;
        }

        error = cls_array_push(&instance->entities, &scene_entity);
        if (error) {
            cls_ecs_scene_entity_destroy(&scene_entity);
            continue;
        }
    }

    *scene = instance;
    return CLS_SUCCESS;

cleanup:
    cls_array_destroy(instance->entities);

    if (instance)
        free(instance);

    return error;
}

int cls_ecs_scene_create_from_query(struct cls_ecs_scene **scene,
                                    struct cls_ecs_world_query *query,
                                    const char *scene_id) {
    if (!scene || !query || !scene_id)
        return CLS_NULLPTR;

    struct cls_ecs_scene *instance = malloc(sizeof(struct cls_ecs_scene));
    if (!scene)
        return CLS_OUT_OF_MEMORY;

    instance->id = scene_id;

    int error = cls_array_create(&instance->entities, 32,
                                 sizeof(struct cls_ecs_scene_entity));
    if (error)
        goto cleanup;

    query->current_index = 0;

    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_ecs_scene_entity scene_entity = {0};
        error = cls_ecs_scene_entity_create(&scene_entity, query->world, e);
        if (error) {
            CLS_LOGGER_LOG(
                CLS_LOGGER_WARN,
                "Saving cls_entity (%u) to scene (%s) from query failed", e,
                scene_id);
            continue;
        }

        error = cls_array_push(&instance->entities, &scene_entity);
        if (error) {
            cls_ecs_scene_entity_destroy(&scene_entity);
            continue;
        }
    }

    *scene = instance;
    return CLS_SUCCESS;

cleanup:
    cls_array_destroy(instance->entities);

    if (instance)
        free(instance);

    return error;
}

void cls_ecs_scene_destroy(struct cls_ecs_scene *scene) {
    if (!scene)
        return;

    if (scene->entities) {
        size_t entity_count = 0;
        cls_array_length_get(&entity_count, scene->entities);

        for (size_t i = 0; i < entity_count; ++i) {
            void *e_ptr = NULL;
            cls_array_elem_get(&e_ptr, scene->entities, i);
            struct cls_ecs_scene_entity *e = e_ptr;
            if (!e || !e->components)
                continue;

            cls_ecs_scene_entity_destroy(e);
        }

        cls_array_destroy(scene->entities);
    }

    free(scene);
}

int cls_ecs_scene_spawn(const struct cls_ecs_scene *scene,
                        struct cls_ecs_world *world) {
    if (!scene || !world)
        return CLS_NULLPTR;

    size_t entities_length = 0;
    int error = cls_array_length_get(&entities_length, scene->entities);
    if (error)
        return error;

    for (size_t e_index = 0; e_index < entities_length; ++e_index) {
        void *entity_ptr = NULL;
        error = cls_array_elem_get(&entity_ptr, scene->entities, e_index);
        if (error)
            continue;

        const struct cls_ecs_scene_entity *scene_entity = entity_ptr;
        if (!scene_entity || !scene_entity->components)
            continue;

        cls_entity e = 0;
        error = cls_ecs_world_entity_add(&e, world);
        if (error) {
            CLS_LOGGER_LOG_ERROR(
                CLS_LOGGER_ERROR, error,
                "Creating cls_entity while spawning scene failed", "%s");
            continue;
        }

        size_t comp_length = 0;
        error = cls_array_length_get(&comp_length, scene_entity->components);
        if (error)
            continue;

        for (size_t comp_index = 0; comp_index < comp_length; ++comp_index) {
            void *comp_ptr = NULL;
            error = cls_array_elem_get(&comp_ptr, scene_entity->components,
                                       comp_index);
            if (error)
                continue;

            const struct cls_ecs_comp_serialized *comp = comp_ptr;
            if (!comp || !comp->comp)
                continue;

            error = cls_ecs_world_component_add_by_id(world, e, comp->id,
                                                      comp->comp);
            if (error) {
                CLS_LOGGER_LOG_ERROR(
                    CLS_LOGGER_ERROR, error,
                    "Adding component (%u) to cls_entity (%u) failed", comp->id,
                    e);
            }
        }
    }

    return CLS_SUCCESS;
}

int cls_ecs_scene_save(const struct cls_ecs_scene *scene, const char *path) {
    if (!scene || !path)
        return CLS_NULLPTR;

    FILE *file = fopen(path, "wb");
    if (!file) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_FILE_NOT_FOUND,
                             "Opening scene file for writing failed: %s", path);
        return CLS_FILE_NOT_FOUND;
    }

    const u32 sig = CLS_ECS_SCENE_FILE_SIG;
    const u32 ver = CLS_ECS_SCENE_FILE_VER;
    (void)fwrite(&sig, sizeof(u32), 1, file);
    (void)fwrite(&ver, sizeof(u32), 1, file);

    size_t e_length = 0;
    int error = cls_array_length_get(&e_length, scene->entities);
    if (error)
        goto cleanup;

    u32 write_e_length = (u32)e_length;
    (void)fwrite(&write_e_length, sizeof(u32), 1, file);

    for (size_t e_index = 0; e_index < e_length; ++e_index) {
        void *e_ptr = NULL;
        error = cls_array_elem_get(&e_ptr, scene->entities, e_index);
        if (error)
            continue;

        const struct cls_ecs_scene_entity *e = e_ptr;
        if (!e || !e->components)
            continue;

        size_t comp_length = 0;
        error = cls_array_length_get(&comp_length, e->components);
        if (error)
            continue;

        u32 write_comp_length = (u32)comp_length;
        (void)fwrite(&write_comp_length, sizeof(u32), 1, file);

        for (size_t comp_index = 0; comp_index < comp_length; ++comp_index) {
            void *comp_ptr = NULL;
            error = cls_array_elem_get(&comp_ptr, e->components, comp_index);
            if (error)
                continue;

            const struct cls_ecs_comp_serialized *comp = comp_ptr;
            if (!comp || !comp->comp)
                continue;

            (void)fwrite(&comp->id, sizeof(u32), 1, file);
            u32 write_size = (u32)comp->size;
            (void)fwrite(&write_size, sizeof(u32), 1, file);
            (void)fwrite(comp->comp, comp->size, 1, file);
        }
    }

    (void)fclose(file);
    return CLS_SUCCESS;

cleanup:
    if (file)
        (void)fclose(file);

    return error;
}

int cls_ecs_scene_load(struct cls_ecs_scene **scene, const char *scene_id,
                       const char *path) {
    if (!scene || !scene_id || !path)
        return CLS_NULLPTR;

    int error = CLS_SUCCESS;
    struct cls_ecs_scene *instance = calloc(1, sizeof(struct cls_ecs_scene));
    FILE *file = fopen(path, "rb");

    if (!scene) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    instance->id = scene_id;

    if (!file) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_FILE_NOT_FOUND,
                             "Opening scene file for reading failed: %s", path);
        error = CLS_FILE_NOT_FOUND;
        goto cleanup;
    }

    u32 sig = 0;
    if (fread(&sig, sizeof(u32), 1, file) != 1 ||
        sig != CLS_ECS_SCENE_FILE_SIG) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, CLS_INVALID_ARG,
                             "Tried to load invalid scene file format: %s",
                             path);
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    u32 ver = 0;
    if (fread(&ver, sizeof(u32), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    if (ver != CLS_ECS_SCENE_FILE_VER) {
        if (ver < CLS_ECS_SCENE_FILE_VER)
            CLS_LOGGER_LOG_ERROR(
                CLS_LOGGER_ERROR, CLS_INVALID_ARG,
                "Scene file version (%u) is too old (unsupported): %s", ver,
                path);

        if (ver > CLS_ECS_SCENE_FILE_VER)
            CLS_LOGGER_LOG_ERROR(
                CLS_LOGGER_ERROR, CLS_INVALID_ARG,
                "Scene file version (%u) is too new (unsupported): %s", ver,
                path);

        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    error = cls_array_create(&instance->entities, 32,
                             sizeof(struct cls_ecs_scene_entity));
    if (error)
        goto cleanup;

    cls_entity e_length = 0;
    if (fread(&e_length, sizeof(u32), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    for (cls_entity e_index = 0; e_index < e_length; ++e_index) {
        struct cls_ecs_scene_entity e = {0};
        error = cls_array_create(&e.components, 8,
                                 sizeof(struct cls_ecs_comp_serialized));
        if (error)
            goto cleanup;

        u32 comp_length = 0;
        if (fread(&comp_length, sizeof(u32), 1, file) != 1) {
            cls_array_destroy(e.components);
            error = CLS_FILE_CORRUPT;
            goto cleanup;
        }

        for (u32 comp_index = 0; comp_index < comp_length; ++comp_index) {
            struct cls_ecs_comp_serialized comp = {0};

            if (fread(&comp.id, sizeof(u32), 1, file) != 1) {
                cls_array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            u32 read_size = 0;
            if (fread(&read_size, sizeof(u32), 1, file) != 1) {
                cls_array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            comp.size = (size_t)read_size;
            comp.comp = malloc(comp.size);
            if (!comp.comp) {
                cls_array_destroy(e.components);
                error = CLS_OUT_OF_MEMORY;
                goto cleanup;
            }

            if (fread(comp.comp, comp.size, 1, file) != 1) {
                free(comp.comp);
                cls_array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            error = cls_array_push(&e.components, &comp);
            if (error) {
                free(comp.comp);
                cls_array_destroy(e.components);
                goto cleanup;
            }
        }

        error = cls_array_push(&instance->entities, &e);
        if (error) {
            cls_array_destroy(e.components);
            goto cleanup;
        }
    }

    (void)fclose(file);
    *scene = instance;
    return CLS_SUCCESS;

cleanup:
    cls_ecs_scene_destroy(instance);

    if (file)
        (void)fclose(file);

    return error;
}

int cls_ecs_world_add(struct cls_ecs *ecs, const char *id, float tick_rate,
                      int priority, bool should_update) {
    if (!ecs || !id)
        return CLS_NULLPTR;

    struct cls_ecs_world world = {0};
    int error = cls_ecs_world_init(&world, tick_rate, priority, should_update);
    if (error)
        return error;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    return cls_table_insert(ecs->worlds, &hash, &world);
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
    cls_ecs_world_destroy(world);
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

int cls_ecs_world_get_all(struct cls_table **worlds,
                          const struct cls_ecs *ecs) {
    if (!worlds || !ecs)
        return CLS_NULLPTR;

    *worlds = ecs->worlds;
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

int cls_ecs_world_entity_add(cls_entity *e, struct cls_ecs_world *world) {
    if (!e || !world)
        return CLS_NULLPTR;

    size_t free_entities_length = 0;
    int error =
        cls_array_length_get(&free_entities_length, world->free_entities);
    if (error)
        return error;

    cls_entity instance = CLS_ENTITY_MAX;
    if (free_entities_length > 0) {
        error = cls_array_elem_get_cpy(&instance, world->free_entities,
                                       free_entities_length - 1);
        if (error)
            return error;

        error = cls_array_pop(NULL, world->free_entities);
        if (error)
            return error;
    } else {
        instance = world->next_entity_id;
        world->next_entity_id++;
    }

    error = cls_array_push(&world->entities, &instance);
    if (error)
        return error;

    *e = instance;
    return CLS_SUCCESS;
}

int cls_ecs_world_entity_remove(struct cls_ecs_world *world, cls_entity e) {
    if (!world)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t pending_deletions_length = 0;
    int error = cls_array_length_get(&pending_deletions_length,
                                     world->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
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
                                     const char *id, size_t component_size) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_ecs_world_sparse_set set = {.component_size = component_size,
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
                             component_size);
    if (error)
        goto cleanup;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

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
    if (!world || !comp)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    return cls_ecs_world_component_add_by_id(world, e, hash, comp);
}

int cls_ecs_world_component_remove(struct cls_ecs_world *world, cls_entity e,
                                   const char *id) {
    if (!world)
        return CLS_NULLPTR;

    if (e == CLS_ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = cls_array_length_get(&entities_length, world->entities);
    if (error)
        return error;

    if (e > entities_length)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    return cls_ecs_world_entity_remove_component_by_id(world, e, hash);
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

    size_t sparse_length = 0;
    error = cls_array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    if (e >= sparse_length)
        return CLS_INVALID_ARG;

    u32 dense_index = 0;
    error = cls_array_elem_get_cpy(&dense_index, set->sparse, e);
    if (error)
        return error;

    size_t dense_length = 0;
    error = cls_array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CLS_INVALID_ARG;

    cls_entity check_entity = CLS_ENTITY_MAX;
    error = cls_array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    return cls_array_elem_get(comp, set->data, dense_index);
}

int cls_ecs_world_query_create(struct cls_ecs_world_query **query,
                               struct cls_ecs_world *world, size_t count, ...) {
    if (!query || !world)
        return CLS_NULLPTR;

    va_list args;
    va_start(args, count);
    int error =
        cls_ecs_world_query_create_from_va_list(query, world, count, args);
    va_end(args);
    if (error)
        return error;

    return CLS_SUCCESS;
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

int cls_ecs_world_query_next(cls_entity *e, struct cls_ecs_world_query *query) {
    if (!e || !query || !query->min_set)
        return CLS_INVALID_ARG;

    *e = CLS_ENTITY_MAX;

    size_t min_dense_length = 0;
    int error = cls_array_length_get(&min_dense_length, query->min_set->dense);
    if (error)
        return error;

    cls_entity current_e = CLS_ENTITY_MAX;
    while (query->current_index < min_dense_length) {
        error = cls_array_elem_get_cpy(&current_e, query->min_set->dense,
                                       query->current_index++);
        if (error != CLS_SUCCESS)
            return error;

        bool match = true;
        for (size_t i = 0; i < query->arg_count; ++i) {
            struct cls_ecs_world_sparse_set *set = NULL;
            error = cls_array_elem_get_cpy((void *)&set, query->sets, i);
            if (error != CLS_SUCCESS) {
                match = false;
                break;
            }

            if (set == query->min_set)
                continue;

            size_t sparse_length = 0;
            error = cls_array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (current_e >= sparse_length) {
                match = false;
                break;
            }

            size_t dense_length = 0;
            error = cls_array_length_get(&dense_length, set->dense);
            if (error)
                return error;

            u32 dense_index = 0;
            error =
                cls_array_elem_get_cpy(&dense_index, set->sparse, current_e);
            if (error != CLS_SUCCESS || dense_index >= dense_length) {
                match = false;
                break;
            }

            u32 check = 0;
            error = cls_array_elem_get_cpy(&check, set->dense, dense_index);
            if (error != CLS_SUCCESS || check != current_e) {
                match = false;
                break;
            }
        }

        if (match) {
            *e = current_e;
            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int cls_ecs_world_query_component_get(void **comp,
                                      const struct cls_ecs_world_query *query,
                                      cls_entity e, const char *id) {
    if (!comp || !query)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        return error;

    for (size_t i = 0; i < query->arg_count; ++i) {
        u32 stored_id = 0;
        error = cls_array_elem_get_cpy(&stored_id, query->comp_ids, i);
        if (error)
            continue;

        if (hash == stored_id) {
            struct cls_ecs_world_sparse_set *set = NULL;
            error = cls_array_elem_get_cpy((void *)&set, query->sets, i);
            if (error)
                return error;

            size_t sparse_length = 0;
            error = cls_array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (e >= sparse_length)
                return CLS_INVALID_ARG;

            u32 dense_index = 0;
            error = cls_array_elem_get_cpy(&dense_index, set->sparse, e);
            if (error)
                return error;

            size_t data_length = 0;
            error = cls_array_length_get(&data_length, set->data);
            if (error)
                return error;

            if (dense_index >= data_length)
                return CLS_INVALID_ARG;

            error = cls_array_elem_get(comp, set->data, dense_index);
            if (error)
                return error;

            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int cls_ecs_world_system_add(struct cls_ecs_world *world, const char *id,
                             cls_ecs_world_system_fn system, void *user_data,
                             size_t user_data_size, size_t query_count, ...) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_ecs_world_system new_system = {.system = system,
                                              .user_data = NULL};
    if (user_data && user_data_size > 0) {
        new_system.user_data = malloc(user_data_size);
        if (!new_system.user_data)
            return CLS_OUT_OF_MEMORY;

        memcpy(new_system.user_data, user_data, user_data_size);
    }

    u32 hash = 0;
    int error = cls_xxhash32(&hash, id, strlen(id), 0);
    if (error)
        goto cleanup;

    if (query_count > 0) {
        va_list args;
        va_start(args, query_count);
        error = cls_ecs_world_query_create_from_va_list(
            &new_system.query, world, query_count, args);
        va_end(args);

        if (error)
            goto cleanup;
    } else {
        new_system.query = NULL;
    }

    error = cls_table_insert(world->systems, &hash, &new_system);
    if (error) {
        if (new_system.query)
            cls_ecs_world_query_destroy(new_system.query);

        goto cleanup;
    }

    return CLS_SUCCESS;

cleanup:
    if (new_system.user_data)
        free(new_system.user_data);

    return error;
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

    free(system->user_data);
    cls_ecs_world_query_destroy(system->query);
    return cls_table_remove(NULL, world->systems, &hash);
}

int cls_ecs_world_update(struct cls_ecs_world *world, struct cls_app *app) {
    if (!world || !app)
        return CLS_NULLPTR;

    if (!world->should_update)
        return CLS_SUCCESS;

    if (world->tick_rate < 0.0f)
        return CLS_SUCCESS;

    if (world->tick_rate < 0.00001f) {
        int error = cls_ecs_world_run_systems(world, app);
        if (error)
            return error;

        return cls_ecs_world_delete_entities(world);
    }

    float dt = 0;
    int error = cls_window_timing_dt_get(&dt, app->window);
    if (error)
        return error;

    float tick_interval = 1.0f / world->tick_rate;
    world->tick_amount += dt;

    while (world->tick_amount >= tick_interval) {
        error = cls_ecs_world_run_systems(world, app);
        if (error)
            return error;

        error = cls_ecs_world_delete_entities(world);
        if (error)
            return error;

        world->tick_amount -= tick_interval;
    }

    return CLS_SUCCESS;
}

int cls_ecs_world_entities_length_get(size_t *len,
                                      const struct cls_ecs_world *world) {
    if (!len || !world)
        return CLS_NULLPTR;

    return cls_array_length_get(len, world->entities);
}
