#include "ecs.h"
#include "core/util/logger.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef ECS_START_CAPACITY
#define ECS_START_CAPACITY 128
#endif

struct comp_pool {
    array *sparse;
    array *dense;
    array *data;
};

struct ecs {
    array *entities;
    array *removed;
    array *comps;
    array *systems;
    ecs_entity next_entity_id;
};

err ecs_new(ecs **out, arena *mem) {
    err err = CORE_SUCCESS;

    if (!out || !mem) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(ecs), _Alignof(ecs));
    if (err)
        goto err;

    (*out)->next_entity_id = 0;

    err = array_new(&(*out)->entities, ECS_START_CAPACITY, sizeof(ecs_entity));
    if (err)
        goto err;

    err = array_new(&(*out)->removed, ECS_START_CAPACITY, sizeof(ecs_entity));
    if (err)
        goto err;

    err =
        array_new(&(*out)->comps, ECS_START_CAPACITY, sizeof(struct comp_pool));
    if (err)
        goto err;

    err = array_new(&(*out)->systems, ECS_START_CAPACITY, sizeof(ecs_system));
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to create entity-comp-systems", err);
    ecs_delete(*out);
    arena_remove_last(mem);
    return err;
}

err ecs_delete(ecs *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    array_delete(in->entities);
    array_delete(in->removed);
    array_delete(in->comps);
    array_delete(in->systems);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete entity-comp-systems", err);
    return err;
}

err ecs_entity_add(ecs_entity *out, ecs *in) {
    err err = CORE_SUCCESS;

    if (!in || !out) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    ecs_entity next_entity = in->next_entity_id;
    size_t removed_length = 0;
    array_length(&removed_length, in->removed);

    if (0 < removed_length) {
        err = array_remove(in->removed, removed_length);
        if (err)
            goto err;
    } else {
        in->next_entity_id++;
    }

    size_t entities_count = 0;
    array_length(&entities_count, in->entities);
    err = array_insert(in->entities, entities_count, &next_entity);
    if (err)
        goto err;

    *out = next_entity;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to add entity", err);
    return err;
}

err ecs_entity_remove(ecs *in, ecs_entity entity) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    err = array_push(in->removed, &entity);
    if (err)
        goto err;

    size_t comps_count = 0;
    array_length(&comps_count, in->comps);

    for (size_t i = 0; i < comps_count; ++i) {
        struct comp_pool *pool = NULL;
        err = array_at((void **)&pool, in->comps, i);
        if (err)
            goto err;

        size_t sparse_index = 0;
        uint32_t dense_index = 0;
        err = array_length(&sparse_index, pool->sparse);
        if (err || entity >= sparse_index)
            continue;

        err = array_at_cpy(&dense_index, pool->sparse, entity);
        if (err)
            continue;

        size_t dense_length = 0;
        array_length(&dense_length, pool->dense);
        if (dense_index >= dense_length)
            continue;

        uint32_t check_entity = 0;
        err = array_at_cpy(&check_entity, pool->dense, dense_index);
        if (err || check_entity != entity)
            continue;

        err = ecs_comp_remove(in, entity, i);
        if (err)
            goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to remove entity", err);
    return err;
}

err ecs_comp_type_add(ecs_comp_type *out, ecs *in, size_t comp_size) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    struct comp_pool pool = {0};

    err = array_new(&pool.sparse, ECS_START_CAPACITY, sizeof(uint32_t));
    if (err)
        goto err;

    err = array_new(&pool.dense, ECS_START_CAPACITY, sizeof(uint32_t));
    if (err)
        goto err;

    err = array_new(&pool.data, ECS_START_CAPACITY, comp_size);
    if (err)
        goto err;

    err = array_push(in->comps, &pool);
    if (err)
        goto err;

    size_t comps_count = 0;
    array_length(&comps_count, in->comps);
    *out = comps_count - 1;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to add comp type", err);
    array_delete(pool.sparse);
    array_delete(pool.dense);
    array_delete(pool.data);
    return err;
}

err ecs_comp_type_remove(ecs *in, ecs_comp_type type) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    struct comp_pool *pool = NULL;
    err = array_at((void **)&pool, in->comps, type);
    if (err)
        goto err;

    array_delete(pool->sparse);
    array_delete(pool->dense);
    array_delete(pool->data);
    array_remove(in->comps, type);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to remove comp type", err);
    return err;
}

err ecs_comp_add(ecs *in, ecs_entity entity, ecs_comp_type type, void *data) {
    err err = CORE_SUCCESS;

    if (!in || !data) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX || type == UINT32_MAX) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    size_t entities_count = 0;
    size_t comps_count = 0;
    array_length(&entities_count, in->entities);
    array_length(&comps_count, in->comps);

    if (entity > entities_count || type > comps_count) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    struct comp_pool *pool = NULL;

    err = array_at((void **)&pool, in->comps, type);
    if (err)
        goto err;

    err = array_insert(pool->sparse, entity, &comps_count);
    if (err)
        goto err;

    err = array_push(pool->dense, &entity);
    if (err)
        goto err;

    err = array_push(pool->data, data);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to add comp to entity", err);
    return err;
}

err ecs_comp_remove(ecs *in, ecs_entity entity, ecs_comp_type type) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX || type == UINT32_MAX) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    size_t entities_count = 0;
    size_t comps_count = 0;
    array_length(&entities_count, in->entities);
    array_length(&comps_count, in->comps);

    if (entity > entities_count || type > comps_count) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    struct comp_pool *pool = NULL;
    err = array_at((void *)pool, in->comps, type);
    if (err)
        goto err;

    size_t dense_length = 0;
    array_length(&dense_length, pool->dense);

    size_t last_index = dense_length--;
    size_t dense_index = UINT32_MAX;
    size_t temp_dense = UINT32_MAX;
    void *temp_data = NULL;

    err = array_at_cpy(&temp_dense, pool->dense, last_index);
    if (err)
        goto err;

    err = array_at_cpy(temp_data, pool->data, last_index);
    if (err)
        goto err;

    err = array_at_cpy(&dense_index, pool->sparse, entity);
    if (err)
        goto err;

    err = array_insert(pool->dense, dense_index, &temp_dense);
    if (err)
        goto err;

    err = array_insert(pool->data, dense_index, &temp_data);
    if (err)
        goto err;

    err = array_insert(pool->sparse, entity, &dense_index);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to remove comp from entity", err);
    return err;
}

err ecs_comps_get(array **out, ecs *in, ecs_comp_type type) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    struct comp_pool *pool = NULL;
    err = array_at((void **)&pool, in->comps, type);
    if (err)
        goto err;

    *out = pool->data;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get comps", err);
    return err;
}

err ecs_system_add(ecs_system_type *out, ecs *in, ecs_system system) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = array_push(in->systems, &system);
    if (err)
        goto err;

    size_t count = 0;
    array_length(&count, in->systems);
    *out = count--;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to add systems", err);
    return err;
}

err ecs_system_remove(ecs *in, ecs_system_type type) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = array_remove(in->systems, type);
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to remove system", err);
    return err;
}

err ecs_update(ecs *in, ecs_ctx *ctx) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    size_t count = 0;
    array_length(&count, in->systems);

    for (size_t i = 0; i < count; ++i) {
        ecs_system system = NULL;
        err = array_at_cpy(&system, in->systems, i);
        if (err)
            goto err;

        err = system(in, ctx);
        if (err)
            goto err;
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to update systems", err);
    return err;
}
