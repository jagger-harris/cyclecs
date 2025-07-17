#include "core/ecs/ecs.h"
#include "core/util/logger.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ECS_START_CAPACITY 128

struct comp_pool {
    struct array sparse;
    struct array dense;
    struct array data;
};

struct ecs_system {
    ecs_system_fn update;
    void *ctx;
};

err ecs_init(struct ecs *out) {
    err status = CORE_SUCCESS;

    if (!out) {
        status = CORE_NULLPTR;
        goto err;
    }

    out->handles = NULL;
    out->next_entity_id = 0;

    status = array_init(&out->entities, ECS_START_CAPACITY, sizeof(uint32_t));
    if (status)
        goto err;

    status = array_init(&out->removed, ECS_START_CAPACITY, sizeof(uint32_t));
    if (status)
        goto err;

    status =
        array_init(&out->comps, ECS_START_CAPACITY, sizeof(struct comp_pool));
    if (status)
        goto err;

    status = array_init(&out->systems, ECS_START_CAPACITY,
                        sizeof(struct ecs_system));
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Creating new ecs failed");
    ecs_destroy(out);
    return status;
}

void ecs_destroy(struct ecs *in) {
    if (!in)
        return;

    if (in->handles)
        free(in->handles);

    array_destroy(&in->entities);
    array_destroy(&in->removed);
    array_destroy(&in->comps);
    array_destroy(&in->systems);
}

err ecs_init_handles(struct ecs *in, void *handles, size_t handles_size) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    // TODO: Use arena or other mem alloc
    in->handles = malloc(handles_size);
    if (!in->handles) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    memcpy(in->handles, handles, handles_size);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Initializing ecs handles failed");
    return status;
}

err ecs_entity_add(uint32_t *out, struct ecs *in) {
    err status = CORE_SUCCESS;

    if (!in || !out) {
        status = CORE_NULLPTR;
        goto err;
    }

    uint32_t next_entity = in->next_entity_id;
    size_t removed_count = in->removed.length;

    if (0 < removed_count) {
        status = array_remove(&in->removed, removed_count);
        if (status)
            goto err;
    } else {
        in->next_entity_id++;
    }

    size_t entities_count = in->entities.length;
    status = array_insert(&in->entities, entities_count, &next_entity);
    if (status)
        goto err;

    *out = next_entity;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Adding entity to ecs failed");
    return status;
}

err ecs_entity_remove(struct ecs *in, uint32_t entity) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX) {
        status = CORE_ARGS;
        goto err;
    }

    status = array_push(&in->removed, &entity);
    if (status)
        goto err;

    size_t comps_length = in->comps.length;

    for (size_t i = 0; i < comps_length; ++i) {
        struct comp_pool *pool = NULL;
        status = array_get_ptr((void **)&pool, &in->comps, i);
        if (status)
            goto err;

        size_t sparse_index = pool->sparse.length;
        uint32_t dense_index = 0;
        if (entity >= sparse_index)
            continue;

        status = array_get_cpy(&dense_index, &pool->sparse, entity);
        if (status)
            continue;

        size_t dense_length = pool->dense.length;
        if (dense_index >= dense_length)
            continue;

        uint32_t check_entity = 0;
        status = array_get_cpy(&check_entity, &pool->dense, dense_index);
        if (status || check_entity != entity)
            continue;

        status = ecs_comp_remove(in, entity, i);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Removing entity from ecs failed");
    return status;
}

err ecs_comp_type_add(uint32_t *out, struct ecs *in, size_t comp_size) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct comp_pool pool = {0};

    status = array_init(&pool.sparse, ECS_START_CAPACITY, sizeof(uint32_t));
    if (status)
        goto err;

    status = array_init(&pool.dense, ECS_START_CAPACITY, sizeof(uint32_t));
    if (status)
        goto err;

    status = array_init(&pool.data, ECS_START_CAPACITY, comp_size);
    if (status)
        goto err;

    status = array_push(&in->comps, &pool);
    if (status)
        goto err;

    size_t comps_count = in->comps.length;
    *out = comps_count - 1;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Adding comp type to ecs failed");
    array_destroy(&pool.sparse);
    array_destroy(&pool.dense);
    array_destroy(&pool.data);
    return status;
}

err ecs_comp_type_remove(struct ecs *in, uint32_t type) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct comp_pool *pool = NULL;
    status = array_get_ptr((void **)&pool, &in->comps, type);
    if (status)
        goto err;

    array_destroy(&pool->sparse);
    array_destroy(&pool->dense);
    array_destroy(&pool->data);
    array_remove(&in->comps, type);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Removing comp type from ecs failed");
    return status;
}

err ecs_comp_add(struct ecs *in, uint32_t entity, uint32_t type, void *data) {
    err status = CORE_SUCCESS;

    if (!in || !data) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX || type == UINT32_MAX) {
        status = CORE_ARGS;
        goto err;
    }

    size_t entities_count = in->entities.length;
    size_t comps_count = in->comps.length;

    if (entity > entities_count || type > comps_count) {
        status = CORE_ARGS;
        goto err;
    }

    struct comp_pool *pool = NULL;

    status = array_get_ptr((void **)&pool, &in->comps, type);
    if (status)
        goto err;

    status = array_insert(&pool->sparse, entity, &comps_count);
    if (status)
        goto err;

    status = array_push(&pool->dense, &entity);
    if (status)
        goto err;

    status = array_push(&pool->data, data);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Adding component to entity failed");
    return status;
}

err ecs_comp_remove(struct ecs *in, uint32_t entity, uint32_t type) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (entity == UINT32_MAX || type == UINT32_MAX) {
        status = CORE_ARGS;
        goto err;
    }

    size_t entities_count = in->entities.length;
    size_t comps_count = in->comps.length;

    if (entity > entities_count || type > comps_count) {
        status = CORE_ARGS;
        goto err;
    }

    struct comp_pool *pool = NULL;
    status = array_get_ptr((void **)&pool, &in->comps, type);
    if (status)
        goto err;

    size_t dense_length = pool->dense.length;
    size_t last_index = dense_length--;
    size_t dense_index = UINT32_MAX;
    size_t temp_dense = UINT32_MAX;
    void *temp_data = NULL;

    status = array_get_cpy(&temp_dense, &pool->dense, last_index);
    if (status)
        goto err;

    status = array_get_cpy(temp_data, &pool->data, last_index);
    if (status)
        goto err;

    status = array_get_cpy(&dense_index, &pool->sparse, entity);
    if (status)
        goto err;

    status = array_insert(&pool->dense, dense_index, &temp_dense);
    if (status)
        goto err;

    status = array_insert(&pool->data, dense_index, &temp_data);
    if (status)
        goto err;

    status = array_insert(&pool->sparse, entity, &dense_index);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Removing comp from entity failed");
    return status;
}

err ecs_comps_get(struct array **out, const struct ecs *in, uint32_t type) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct comp_pool *pool = NULL;
    status = array_get_ptr((void **)&pool, &in->comps, type);
    if (status)
        goto err;

    *out = &pool->data;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Getting ecs comps failed");
    return status;
}

err ecs_system_add(uint32_t *out, struct ecs *in, ecs_system_fn fn, void *ctx) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct ecs_system new_system = {
        .ctx = ctx,
        .update = fn,
    };

    status = array_push(&in->systems, &new_system);
    if (status)
        goto err;

    size_t count = in->systems.length;
    *out = count--;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Adding sys to ecs failed");
    return status;
}

err ecs_system_remove(struct ecs *in, uint32_t type) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = array_remove(&in->systems, type);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Removing sys from ecs failed");
    return status;
}

err ecs_update_all(const struct ecs *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    size_t count = in->systems.length;

    for (size_t i = 0; i < count; ++i) {
        struct ecs_system *system = NULL;
        status = array_get_ptr((void **)&system, &in->systems, i);
        if (status)
            goto err;

        status = system->update(in, system->ctx);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Updating all ecs systems failed");
    return status;
}
