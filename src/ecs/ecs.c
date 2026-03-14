#include <cls/app/app.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/util/allocator.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define ECS_WORLD_START_CAPACITY 4
#define ECS_WORLD_ENTITY_START_CAPACITY 32
#define ECS_SCENE_FILE_SIG 0xE7EFD7A7 // çï×§ (cyclecs scene)
#define ECS_SCENE_FILE_VER 1

struct ecs {
    struct table *worlds;
};

struct ecs_scene {
    const char *id;
    struct array *entities;
};

struct ecs_scene_entity {
    struct array *components; // Entity can have more than one component
};

// TODO: Naive approach. Would be better to make a specific data structure for
// more efficient serialization
struct ecs_comp_serialized {
    u32 comp_id;
    size_t size;
    void *data;
};

struct ecs_world_query {
    struct array *sets;
    struct array *comp_ids;
    struct ecs_world_sparse_set *min_set;
    struct ecs_world *world;
    size_t arg_count;
    size_t current_index;
};

struct ecs_world_system {
    struct ecs_world_query query;
    ecs_world_system_fn system;
    void *user_data;
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
    while (table_iterator_next(&comp_iter_next, comp_iter) == CLS_SUCCESS &&
           comp_iter_next) {
        void *set_ptr = NULL;
        error = table_iterator_value_get(&set_ptr, comp_iter);
        if (error)
            continue;

        struct ecs_world_sparse_set *set = set_ptr;
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
    while (table_iterator_next(&sys_iter_next, sys_iter) == CLS_SUCCESS &&
           sys_iter_next) {
        void *system_ptr = NULL;
        error = table_iterator_value_get(&system_ptr, sys_iter);
        if (error)
            continue;

        struct ecs_world_system *system = system_ptr;
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
        return CLS_NULLPTR;

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

    return CLS_SUCCESS;

cleanup:
    ecs_world_destroy(out);
    return error;
}

int ecs_create(struct ecs **out, struct allocator *alloc) {
    if (!out)
        return CLS_NULLPTR;

    struct ecs *ecs = NULL;
    int error = allocator_alloc((void **)&ecs, alloc, sizeof(struct ecs),
                                alignof(struct ecs));
    if (error)
        return error;

    error = table_create(&ecs->worlds, ECS_WORLD_START_CAPACITY, sizeof(u32),
                         sizeof(struct ecs_world));
    if (error)
        goto cleanup;

    *out = ecs;
    return CLS_SUCCESS;

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
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *world_ptr = NULL;
        error = table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct ecs_world *world = world_ptr;
        ecs_world_destroy(world);
    }

    table_iterator_destroy(iter);
    table_destroy(in->worlds);
}

static void ecs_scene_entity_destroy(struct ecs_scene_entity *in) {
    if (!in || !in->components)
        return;

    size_t comp_length = 0;
    array_length_get(&comp_length, in->components);

    for (size_t i = 0; i < comp_length; ++i) {
        void *comp_ptr = NULL;
        array_elem_get(&comp_ptr, in->components, i);
        struct ecs_comp_serialized *comp = comp_ptr;
        if (comp && comp->data)
            free(comp->data);
    }

    array_destroy(in->components);
}

static int ecs_scene_entity_create(struct ecs_scene_entity *out,
                                   const struct ecs_world *world, entity e) {
    if (!out || !world)
        return CLS_NULLPTR;

    int error =
        array_create(&out->components, 8, sizeof(struct ecs_comp_serialized));
    if (error)
        return error;

    struct table_iterator *iter = NULL;
    error = table_iterator_create(&iter, world->components);
    if (error)
        goto cleanup;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *comp_id_ptr = NULL;
        error = table_iterator_key_get(&comp_id_ptr, iter);
        if (error)
            continue;

        u32 *comp_id = comp_id_ptr;

        void *set_ptr = NULL;
        error = table_iterator_value_get(&set_ptr, iter);
        if (error)
            continue;

        const struct ecs_world_sparse_set *set = set_ptr;
        if (!set)
            continue;

        size_t sparse_length = 0;
        error = array_length_get(&sparse_length, set->sparse);
        if (error)
            continue;

        u32 dense_index = 0;
        error = array_elem_get_cpy(&dense_index, set->sparse, e);
        if (error)
            continue;

        size_t dense_length = 0;
        error = array_length_get(&dense_length, set->dense);
        if (error || dense_index >= dense_length)
            continue;

        u32 check_entity = 0;
        error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
        if (error || check_entity != e)
            continue;

        void *comp_data = NULL;
        error = array_elem_get(&comp_data, set->data, dense_index);
        if (error)
            continue;

        // TODO: Use arena alloc
        void *comp_data_cpy = malloc(set->component_size);
        if (!comp_data_cpy)
            goto cleanup;

        memcpy(comp_data_cpy, comp_data, set->component_size);

        struct ecs_comp_serialized saved = {.comp_id = *comp_id,
                                            .size = set->component_size,
                                            .data = comp_data_cpy};

        error = array_push(&out->components, &saved);
        if (error)
            goto cleanup;
    }

    table_iterator_destroy(iter);
    return CLS_SUCCESS;

cleanup:
    ecs_scene_entity_destroy(out);
    table_iterator_destroy(iter);
    return error;
}

int ecs_scene_create_from_world(struct ecs_scene **out,
                                const struct ecs_world *in,
                                const char *scene_id) {
    if (!out || !in || !scene_id)
        return CLS_NULLPTR;

    struct ecs_scene *scene = malloc(sizeof(struct ecs_scene));
    if (!scene)
        return CLS_OUT_OF_MEMORY;

    scene->id = scene_id;

    int error =
        array_create(&scene->entities, 32, sizeof(struct ecs_scene_entity));
    if (error)
        goto cleanup;

    size_t e_length = 0;
    error = ecs_world_entities_length_get(&e_length, in);
    if (error)
        goto cleanup;

    for (size_t i = 0; i < e_length; ++i) {
        entity e = 0;
        error = array_elem_get_cpy(&e, in->entities, i);
        if (error)
            continue;

        struct ecs_scene_entity scene_entity = {0};
        error = ecs_scene_entity_create(&scene_entity, in, e);
        if (error) {
            LOGGER_LOG(LOGGER_WARN,
                       "Saving entity (%u) to scene (%s) from world failed", e,
                       scene_id);
            continue;
        }

        error = array_push(&scene->entities, &scene_entity);
        if (error) {
            ecs_scene_entity_destroy(&scene_entity);
            continue;
        }
    }

    *out = scene;
    return CLS_SUCCESS;

cleanup:
    array_destroy(scene->entities);

    if (scene)
        free(scene);

    return error;
}

int ecs_scene_create_from_query(struct ecs_scene **out,
                                struct ecs_world_query *in,
                                const char *scene_id) {
    if (!out || !in || !scene_id)
        return CLS_NULLPTR;

    struct ecs_scene *scene = malloc(sizeof(struct ecs_scene));
    if (!scene)
        return CLS_OUT_OF_MEMORY;

    scene->id = scene_id;

    int error =
        array_create(&scene->entities, 32, sizeof(struct ecs_scene_entity));
    if (error)
        goto cleanup;

    in->current_index = 0;

    entity e = ENTITY_MAX;
    while (ecs_world_query_next(&e, in) == CLS_SUCCESS && e != ENTITY_MAX) {
        struct ecs_scene_entity scene_entity = {0};
        error = ecs_scene_entity_create(&scene_entity, in->world, e);
        if (error) {
            LOGGER_LOG(LOGGER_WARN,
                       "Saving entity (%u) to scene (%s) from query failed", e,
                       scene_id);
            continue;
        }

        error = array_push(&scene->entities, &scene_entity);
        if (error) {
            ecs_scene_entity_destroy(&scene_entity);
            continue;
        }
    }

    *out = scene;
    return CLS_SUCCESS;

cleanup:
    array_destroy(scene->entities);

    if (scene)
        free(scene);

    return error;
}

void ecs_scene_destroy(struct ecs_scene *in) {
    if (!in)
        return;

    if (in->entities) {
        size_t entity_count = 0;
        array_length_get(&entity_count, in->entities);

        for (size_t i = 0; i < entity_count; ++i) {
            void *e_ptr = NULL;
            array_elem_get(&e_ptr, in->entities, i);
            struct ecs_scene_entity *e = e_ptr;
            if (!e || !e->components)
                continue;

            ecs_scene_entity_destroy(e);
        }

        array_destroy(in->entities);
    }

    free(in);
}

int ecs_scene_spawn(const struct ecs_scene *in, struct ecs_world *world) {
    if (!in || !world)
        return CLS_NULLPTR;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    for (size_t e_index = 0; e_index < entities_length; ++e_index) {
        void *entity_ptr = NULL;
        error = array_elem_get(&entity_ptr, in->entities, e_index);
        if (error)
            continue;

        const struct ecs_scene_entity *scene_entity = entity_ptr;
        if (!scene_entity || !scene_entity->components)
            continue;

        entity e = 0;
        error = ecs_world_entity_add(&e, world);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                             "Creating entity while spawning scene failed",
                             "%s");
            continue;
        }

        size_t comp_length = 0;
        error = array_length_get(&comp_length, scene_entity->components);
        if (error)
            continue;

        for (size_t comp_index = 0; comp_index < comp_length; ++comp_index) {
            void *comp_ptr = NULL;
            error =
                array_elem_get(&comp_ptr, scene_entity->components, comp_index);
            if (error)
                continue;

            const struct ecs_comp_serialized *comp = comp_ptr;
            if (!comp || !comp->data)
                continue;

            error =
                ecs_world_component_add(world, e, comp->comp_id, comp->data);
            if (error) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                                 "Adding component (%u) to entity (%u) failed",
                                 comp->comp_id, e);
            }
        }
    }

    return CLS_SUCCESS;
}

int ecs_scene_save(const struct ecs_scene *in, const char *path) {
    if (!in || !path)
        return CLS_NULLPTR;

    FILE *file = fopen(path, "wb");
    if (!file) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_FILE_NOT_FOUND,
                         "Opening scene file for writing failed: %s", path);
        return CLS_FILE_NOT_FOUND;
    }

    const u32 sig = ECS_SCENE_FILE_SIG;
    const u32 ver = ECS_SCENE_FILE_VER;
    (void)fwrite(&sig, sizeof(u32), 1, file);
    (void)fwrite(&ver, sizeof(u32), 1, file);

    size_t e_length = 0;
    int error = array_length_get(&e_length, in->entities);
    if (error)
        goto cleanup;

    u32 write_e_length = (u32)e_length;
    (void)fwrite(&write_e_length, sizeof(u32), 1, file);

    for (size_t e_index = 0; e_index < e_length; ++e_index) {
        void *e_ptr = NULL;
        error = array_elem_get(&e_ptr, in->entities, e_index);
        if (error)
            continue;

        const struct ecs_scene_entity *e = e_ptr;
        if (!e || !e->components)
            continue;

        size_t comp_length = 0;
        error = array_length_get(&comp_length, e->components);
        if (error)
            continue;

        u32 write_comp_length = (u32)comp_length;
        (void)fwrite(&write_comp_length, sizeof(u32), 1, file);

        for (size_t comp_index = 0; comp_index < comp_length; ++comp_index) {
            void *comp_ptr = NULL;
            error = array_elem_get(&comp_ptr, e->components, comp_index);
            if (error)
                continue;

            const struct ecs_comp_serialized *comp = comp_ptr;
            if (!comp || !comp->data)
                continue;

            (void)fwrite(&comp->comp_id, sizeof(u32), 1, file);
            u32 write_size = (u32)comp->size;
            (void)fwrite(&write_size, sizeof(u32), 1, file);
            (void)fwrite(comp->data, comp->size, 1, file);
        }
    }

    (void)fclose(file);
    return CLS_SUCCESS;

cleanup:
    if (file)
        (void)fclose(file);

    return error;
}

int ecs_scene_load(struct ecs_scene **out, const char *scene_id,
                   const char *path) {
    if (!out || !scene_id || !path)
        return CLS_NULLPTR;

    int error = CLS_SUCCESS;
    struct ecs_scene *scene = calloc(1, sizeof(struct ecs_scene));
    FILE *file = fopen(path, "rb");

    if (!scene) {
        error = CLS_OUT_OF_MEMORY;
        goto cleanup;
    }

    scene->id = scene_id;

    if (!file) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_FILE_NOT_FOUND,
                         "Opening scene file for reading failed: %s", path);
        error = CLS_FILE_NOT_FOUND;
        goto cleanup;
    }

    u32 sig = 0;
    if (fread(&sig, sizeof(u32), 1, file) != 1 || sig != ECS_SCENE_FILE_SIG) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CLS_INVALID_ARG,
                         "Tried to load invalid scene file format: %s", path);
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    u32 ver = 0;
    if (fread(&ver, sizeof(u32), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    if (ver != ECS_SCENE_FILE_VER) {
        if (ver < ECS_SCENE_FILE_VER)
            LOGGER_LOG_ERROR(
                LOGGER_ERROR, CLS_INVALID_ARG,
                "Scene file version (%u) is too old (unsupported): %s", ver,
                path);

        if (ver > ECS_SCENE_FILE_VER)
            LOGGER_LOG_ERROR(
                LOGGER_ERROR, CLS_INVALID_ARG,
                "Scene file version (%u) is too new (unsupported): %s", ver,
                path);

        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    error = array_create(&scene->entities, 32, sizeof(struct ecs_scene_entity));
    if (error)
        goto cleanup;

    entity e_length = 0;
    if (fread(&e_length, sizeof(u32), 1, file) != 1) {
        error = CLS_FILE_CORRUPT;
        goto cleanup;
    }

    for (entity e_index = 0; e_index < e_length; ++e_index) {
        struct ecs_scene_entity e = {0};
        error =
            array_create(&e.components, 8, sizeof(struct ecs_comp_serialized));
        if (error)
            goto cleanup;

        u32 comp_length = 0;
        if (fread(&comp_length, sizeof(u32), 1, file) != 1) {
            array_destroy(e.components);
            error = CLS_FILE_CORRUPT;
            goto cleanup;
        }

        for (u32 comp_index = 0; comp_index < comp_length; ++comp_index) {
            struct ecs_comp_serialized comp = {0};

            if (fread(&comp.comp_id, sizeof(u32), 1, file) != 1) {
                array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            u32 read_size = 0;
            if (fread(&read_size, sizeof(u32), 1, file) != 1) {
                array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            comp.size = (size_t)read_size;
            comp.data = malloc(comp.size);
            if (!comp.data) {
                array_destroy(e.components);
                error = CLS_OUT_OF_MEMORY;
                goto cleanup;
            }

            if (fread(comp.data, comp.size, 1, file) != 1) {
                free(comp.data);
                array_destroy(e.components);
                error = CLS_FILE_CORRUPT;
                goto cleanup;
            }

            error = array_push(&e.components, &comp);
            if (error) {
                free(comp.data);
                array_destroy(e.components);
                goto cleanup;
            }
        }

        error = array_push(&scene->entities, &e);
        if (error) {
            array_destroy(e.components);
            goto cleanup;
        }
    }

    (void)fclose(file);
    *out = scene;
    return CLS_SUCCESS;

cleanup:
    ecs_scene_destroy(scene);

    if (file)
        (void)fclose(file);

    return error;
}

int ecs_world_add(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update) {
    if (!in)
        return CLS_NULLPTR;

    struct ecs_world world = {0};
    int error = ecs_world_init(&world, tick_rate, priority, should_update);
    if (error)
        return error;

    return table_insert(in->worlds, &world_id, &world);
}

int ecs_world_remove(struct ecs *in, u32 world_id) {
    if (!in || !world_id)
        return CLS_NULLPTR;

    void *world_ptr = NULL;
    int error = table_find(&world_ptr, in->worlds, &world_id);
    if (error)
        return error;

    struct ecs_world *world = world_ptr;
    ecs_world_destroy(world);
    return table_remove(NULL, in->worlds, &world_id);
}

int ecs_world_get(struct ecs_world **out, const struct ecs *in, u32 world_id) {
    if (!out || !in)
        return CLS_NULLPTR;

    void *world_ptr = NULL;
    int error = table_find(&world_ptr, in->worlds, &world_id);
    if (error)
        return error;

    *out = world_ptr;
    return CLS_SUCCESS;
}

int ecs_world_get_all(struct table **out, const struct ecs *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->worlds;
    return CLS_SUCCESS;
}

int ecs_world_update_all(struct ecs *in, struct app *app) {
    if (!in)
        return CLS_NULLPTR;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *world_ptr = NULL;
        error = table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct ecs_world *world = world_ptr;
        if (!world)
            continue;

        void *key_ptr = NULL;
        error = table_iterator_key_get(&key_ptr, iter);
        if (error)
            continue;

        int *key = key_ptr;
        error = ecs_world_update(world, app);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                             "Updating ecs world failed (%i)", *key);
            continue;
        }
    }

    table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

int ecs_world_iter_all(struct ecs *in, ecs_world_iter_callback callback,
                       void *user_data) {
    if (!in || !callback)
        return CLS_NULLPTR;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->worlds);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *world_ptr = NULL;
        error = table_iterator_value_get(&world_ptr, iter);
        if (error)
            continue;

        struct ecs_world *world = world_ptr;
        if (!world)
            continue;

        error = callback(world, user_data);
        if (error)
            continue;
    }

    table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

int ecs_world_entity_add(u32 *out, struct ecs_world *in) {
    if (!in || !out)
        return CLS_NULLPTR;

    size_t free_entities_length = 0;
    int error = array_length_get(&free_entities_length, in->free_entities);
    if (error)
        return error;

    entity e = ENTITY_MAX;
    if (free_entities_length > 0) {
        error =
            array_elem_get_cpy(&e, in->free_entities, free_entities_length - 1);
        if (error)
            return error;

        error = array_pop(NULL, in->free_entities);
        if (error)
            return error;
    } else {
        e = in->next_entity_id;
        in->next_entity_id++;
    }

    error = array_push(&in->entities, &e);
    if (error)
        return error;

    *out = e;
    return CLS_SUCCESS;
}

static int ecs_world_entity_remove_component(struct ecs_world *in, entity e,
                                             u32 comp_id) {
    if (!in)
        return CLS_NULLPTR;

    void *set_ptr = NULL;
    int error = table_find(&set_ptr, in->components, &comp_id);
    if (error)
        return error;

    const struct ecs_world_sparse_set *set = set_ptr;

    size_t dense_length = 0;
    error = array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_length == 0)
        return CLS_SUCCESS;

    u32 dense_index = 0;
    error = array_elem_get_cpy(&dense_index, set->sparse, e);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CLS_INVALID_ARG;

    u32 check_entity = 0;
    error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    size_t last_index = dense_length - 1;
    if (dense_index != last_index) {
        u32 last_entity = 0;

        error = array_elem_get_cpy(&last_entity, set->dense, last_index);
        if (error)
            return error;

        void *last_data = NULL;
        error = array_elem_get(&last_data, set->data, last_index);
        if (error)
            return error;

        void *current_data = NULL;
        error = array_elem_get(&current_data, set->data, dense_index);
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

    error = array_pop(NULL, set->dense);
    if (error)
        return error;

    error = array_pop(NULL, set->data);
    if (error)
        return error;

    entity invalid_e = ENTITY_MAX;
    return array_elem_set(set->sparse, e, &invalid_e);
}

int ecs_world_entity_remove(struct ecs_world *in, entity e) {
    if (!in)
        return CLS_NULLPTR;

    if (e == ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t pending_deletions_length = 0;
    int error =
        array_length_get(&pending_deletions_length, in->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
        entity pending = ENTITY_MAX;
        error = array_elem_get_cpy(&pending, in->pending_deletions, i);
        if (error)
            continue;

        if (pending == e)
            return CLS_SUCCESS;
    }

    return array_push(&in->pending_deletions, &e);
}

int ecs_world_component_type_add(struct ecs_world *in, u32 comp_id,
                                 size_t component_size) {
    if (!in)
        return CLS_NULLPTR;

    struct ecs_world_sparse_set set = {.component_size = component_size,
                                       .sparse = NULL,
                                       .dense = NULL,
                                       .data = NULL};
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

    error = table_insert(in->components, &comp_id, &set);
    if (error)
        goto cleanup;

    return CLS_SUCCESS;

cleanup:
    array_destroy(set.sparse);
    array_destroy(set.dense);
    array_destroy(set.data);
    return error;
}

int ecs_world_component_type_remove(struct ecs_world *in, u32 comp_id) {
    if (!in)
        return CLS_NULLPTR;

    void *set_ptr = NULL;
    int error = table_find(&set_ptr, in->components, &comp_id);
    if (error)
        return error;

    const struct ecs_world_sparse_set *set = set_ptr;
    array_destroy(set->sparse);
    array_destroy(set->dense);
    array_destroy(set->data);
    return table_remove(NULL, in->components, &comp_id);
}

int ecs_world_component_add(struct ecs_world *in, entity e, u32 comp_id,
                            const void *comp_data) {
    if (!in || !comp_data)
        return CLS_NULLPTR;

    if (e == ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (e >= entities_length)
        return CLS_INVALID_ARG;

    void *set_ptr = NULL;
    error = table_find(&set_ptr, in->components, &comp_id);
    if (error)
        return error;

    struct ecs_world_sparse_set *set = set_ptr;
    if (!set)
        return CLS_INVALID_ARG;

    size_t sparse_length = 0;
    error = array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    while (sparse_length <= e) {
        entity invalid_e = ENTITY_MAX;
        error = array_push(&set->sparse, &invalid_e);
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

    error = array_elem_set(set->sparse, (size_t)e, &dense_length);
    if (error)
        return error;

    error = array_push(&set->dense, &e);
    if (error)
        return error;

    return array_push(&set->data, comp_data);
}

int ecs_world_component_remove(struct ecs_world *in, entity e, u32 comp_id) {
    if (!in)
        return CLS_NULLPTR;

    if (e == ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (e > entities_length)
        return CLS_INVALID_ARG;

    return ecs_world_entity_remove_component(in, e, comp_id);
}

int ecs_world_component_get(void **out, const struct ecs_world *in, entity e,
                            u32 comp_id) {
    if (!out || !in)
        return CLS_NULLPTR;

    if (e == ENTITY_MAX)
        return CLS_INVALID_ARG;

    void *set_ptr = NULL;
    int error = table_find(&set_ptr, in->components, &comp_id);
    if (error)
        return error;

    const struct ecs_world_sparse_set *set = set_ptr;
    if (!set) {
        LOGGER_LOG_ERROR(
            LOGGER_ERROR, error,
            "ECS query failed because component type (%u) does not exist",
            comp_id);
        return CLS_INVALID_ARG;
    }

    size_t sparse_length = 0;
    error = array_length_get(&sparse_length, set->sparse);
    if (error)
        return error;

    if (e >= sparse_length)
        return CLS_INVALID_ARG;

    u32 dense_index = 0;
    error = array_elem_get_cpy(&dense_index, set->sparse, e);
    if (error)
        return error;

    size_t dense_length = 0;
    error = array_length_get(&dense_length, set->dense);
    if (error)
        return error;

    if (dense_index >= dense_length)
        return CLS_INVALID_ARG;

    u32 check_entity = 0;
    error = array_elem_get_cpy(&check_entity, set->dense, dense_index);
    if (error)
        return error;

    if (check_entity != e)
        return CLS_INVALID_ARG;

    return array_elem_get(out, set->data, dense_index);
}

static int ecs_world_query_create_from_va_list(struct ecs_world_query *out,
                                               struct ecs_world *world,
                                               size_t arg_count, va_list args) {
    if (!out || !world || arg_count == 0)
        return CLS_INVALID_ARG;

    out->world = world;
    out->arg_count = arg_count;
    out->min_set = NULL;
    out->current_index = 0;

    int error = array_create(&out->comp_ids, arg_count, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_create(&out->sets, arg_count,
                         sizeof(struct ecs_world_sparse_set *));
    if (error)
        goto cleanup;

    for (size_t i = 0; i < arg_count; ++i) {
        u32 comp_id = va_arg(args, u32);

        error = array_push(&out->comp_ids, &comp_id);
        if (error)
            continue;

        void *set_ptr = NULL;
        error = table_find(&set_ptr, world->components, &comp_id);
        if (error)
            continue;

        if (!set_ptr)
            continue;

        error = array_push(&out->sets, (const void *)&set_ptr);
        if (error)
            continue;
    }

    void *set_ptr = NULL;
    error = array_elem_get(&set_ptr, out->sets, 0);
    if (error)
        goto cleanup;

    struct ecs_world_sparse_set *first_set =
        *(struct ecs_world_sparse_set **)set_ptr;
    if (!first_set) {
        error = CLS_INVALID_ARG;
        goto cleanup;
    }

    out->min_set = first_set;

    size_t min_dense_length = 0;
    error = array_length_get(&min_dense_length, out->min_set->dense);
    if (error)
        goto cleanup;

    size_t set_count = 0;
    error = array_length_get(&set_count, out->sets);
    if (error)
        goto cleanup;

    for (size_t i = 1; i < arg_count; ++i) {
        struct ecs_world_sparse_set *current_set = NULL;
        error = array_elem_get_cpy((void *)&current_set, out->sets, i);
        if (error)
            continue;

        size_t dense_length = 0;
        error = array_length_get(&dense_length, current_set->dense);
        if (error)
            continue;

        if (dense_length < min_dense_length)
            out->min_set = current_set;
    }

    return CLS_SUCCESS;

cleanup:
    ecs_world_query_destroy(out);
    return error;
}

int ecs_world_query_create(struct ecs_world_query **out,
                           struct ecs_world *world, size_t count, ...) {
    if (!out || !world)
        return CLS_NULLPTR;

    // TODO: Use arena allocator
    struct ecs_world_query *query = malloc(sizeof(struct ecs_world_query));
    if (!query)
        return CLS_OUT_OF_MEMORY;

    va_list args;
    va_start(args, count);
    int error = ecs_world_query_create_from_va_list(query, world, count, args);
    va_end(args);

    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Error...");
        goto cleanup;
    }

    *out = query;
    return CLS_SUCCESS;
cleanup:
    free(query);
    return error;
}

void ecs_world_query_destroy(struct ecs_world_query *query) {
    if (!query)
        return;

    array_destroy(query->comp_ids);
    array_destroy(query->sets);
}

int ecs_world_query_world_get(struct ecs_world **world,
                              struct ecs_world_query *query) {
    if (!world || !query)
        return CLS_NULLPTR;

    *world = query->world;
    return CLS_SUCCESS;
}

int ecs_world_query_next(entity *out, struct ecs_world_query *query) {
    if (!out || !query || !query->min_set)
        return CLS_INVALID_ARG;

    *out = ENTITY_MAX;

    size_t min_dense_length = 0;
    int error = array_length_get(&min_dense_length, query->min_set->dense);
    if (error)
        return error;

    while (query->current_index < min_dense_length) {
        entity e = 0;
        error = array_elem_get_cpy(&e, query->min_set->dense,
                                   query->current_index++);
        if (error != CLS_SUCCESS)
            return error;

        bool match = true;
        for (size_t i = 0; i < query->arg_count; ++i) {
            struct ecs_world_sparse_set *set = NULL;
            error = array_elem_get_cpy((void *)&set, query->sets, i);
            if (error != CLS_SUCCESS) {
                match = false;
                break;
            }

            if (set == query->min_set)
                continue;

            size_t sparse_length = 0;
            error = array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (e >= sparse_length) {
                match = false;
                break;
            }

            size_t dense_length = 0;
            error = array_length_get(&dense_length, set->dense);
            if (error)
                return error;

            u32 dense_index = 0;
            error = array_elem_get_cpy(&dense_index, set->sparse, e);
            if (error != CLS_SUCCESS || dense_index >= dense_length) {
                match = false;
                break;
            }

            u32 check = 0;
            error = array_elem_get_cpy(&check, set->dense, dense_index);
            if (error != CLS_SUCCESS || check != e) {
                match = false;
                break;
            }
        }

        if (match) {
            *out = e;
            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int ecs_world_query_component_get(void **out,
                                  const struct ecs_world_query *query,
                                  u32 comp_id, entity e) {
    if (!out || !query)
        return CLS_NULLPTR;

    for (size_t i = 0; i < query->arg_count; ++i) {
        u32 stored_id = 0;
        int error = array_elem_get_cpy(&stored_id, query->comp_ids, i);
        if (error)
            continue;

        if (comp_id == stored_id) {
            struct ecs_world_sparse_set *set = NULL;
            error = array_elem_get_cpy((void *)&set, query->sets, i);
            if (error)
                return error;

            size_t sparse_length = 0;
            error = array_length_get(&sparse_length, set->sparse);
            if (error)
                return error;

            if (e >= sparse_length)
                return CLS_INVALID_ARG;

            u32 dense_index = 0;
            error = array_elem_get_cpy(&dense_index, set->sparse, e);
            if (error)
                return error;

            size_t data_length = 0;
            error = array_length_get(&data_length, set->data);
            if (error)
                return error;

            if (dense_index >= data_length)
                return CLS_INVALID_ARG;

            error = array_elem_get(out, set->data, dense_index);
            if (error)
                return error;

            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int ecs_world_system_add(struct ecs_world *in, u32 system_id,
                         ecs_world_system_fn system, void *user_data,
                         size_t query_count, ...) {
    if (!in)
        return CLS_NULLPTR;

    struct ecs_world_system new_system = {.system = system,
                                          .user_data = user_data};

    if (query_count > 0) {
        va_list args;
        va_start(args, query_count);
        int error = ecs_world_query_create_from_va_list(&new_system.query, in,
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

    return CLS_SUCCESS;
}

int ecs_world_system_remove(struct ecs_world *in, u32 system_id) {
    if (!in)
        return CLS_NULLPTR;

    void *system_ptr = NULL;
    int error = table_find(&system_ptr, in->systems, &system_id);
    if (error)
        return error;

    struct ecs_world_system *system = system_ptr;
    if (!system)
        return CLS_NULLPTR;

    ecs_world_query_destroy(&system->query);
    return table_remove(NULL, in->systems, &system_id);
}

static int ecs_world_run_systems(struct ecs_world *in, struct app *app) {
    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, in->systems);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *system_ptr = NULL;
        error = table_iterator_value_get(&system_ptr, iter);
        if (error)
            continue;

        struct ecs_world_system *system = system_ptr;
        if (!system || !system->system)
            continue;

        system->query.current_index = 0;

        error = system->system(&system->query, app, system->user_data);
        if (error)
            return error;
    }

    table_iterator_destroy(iter);
    return CLS_SUCCESS;
}

static int ecs_world_entity_remove_now(struct ecs_world *in, entity e) {
    if (!in)
        return CLS_NULLPTR;

    if (e == ENTITY_MAX)
        return CLS_INVALID_ARG;

    size_t entities_length = 0;
    int error = array_length_get(&entities_length, in->entities);
    if (error)
        return error;

    if (e >= entities_length)
        return CLS_INVALID_ARG;

    struct table_iterator *iter = NULL;
    error = table_iterator_create(&iter, in->components);
    if (error)
        return error;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *set_ptr = NULL;
        error = table_iterator_value_get(&set_ptr, iter);
        if (error)
            continue;

        struct ecs_world_sparse_set *set = set_ptr;
        if (!set)
            continue;

        size_t sparse_length = 0;
        error = array_length_get(&sparse_length, set->sparse);
        if (error)
            return error;

        if (e >= sparse_length)
            continue;

        u32 dense_index = 0;
        error = array_elem_get_cpy(&dense_index, set->sparse, e);
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
        if (error || check_entity != e)
            continue;

        void *key_ptr = NULL;
        error = table_iterator_key_get(&key_ptr, iter);
        if (error)
            continue;

        u32 *key = key_ptr;
        error = ecs_world_entity_remove_component(in, e, *key);
        if (error)
            return error;
    }

    table_iterator_destroy(iter);
    return array_push(&in->free_entities, &e);
}

static int ecs_world_delete_entities(struct ecs_world *in) {
    if (!in)
        return CLS_NULLPTR;

    size_t pending_deletions_length = 0;
    int error =
        array_length_get(&pending_deletions_length, in->pending_deletions);
    if (error)
        return error;

    for (size_t i = 0; i < pending_deletions_length; ++i) {
        entity e = ENTITY_MAX;
        error = array_elem_get_cpy(&e, in->pending_deletions, i);
        if (error)
            continue;

        error = ecs_world_entity_remove_now(in, e);
        if (error)
            LOGGER_LOG(LOGGER_ERROR, "Failed to delete entity %u", e);
    }

    array_clear(in->pending_deletions);
    return CLS_SUCCESS;
}

int ecs_world_update(struct ecs_world *in, struct app *app) {
    if (!in || !app)
        return CLS_NULLPTR;

    if (!in->should_update)
        return CLS_SUCCESS;

    if (in->tick_rate < 0.0f)
        return CLS_SUCCESS;

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

    return CLS_SUCCESS;
}

int ecs_world_entities_length_get(size_t *out, const struct ecs_world *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    return array_length_get(out, in->entities);
}
