#include "core/util/arena.h"
#include "core/util/logger.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

err arena_init(struct arena *out, size_t capacity) {
    err status = CORE_SUCCESS;

    out->mem = malloc(capacity);
    if (!out->mem) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    out->capacity = capacity;
    out->used = 0;
    out->last_offset = 0;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init arena failed");
    return status;
}

void arena_destroy(struct arena *in) {
    if (!in || !in->mem)
        return;

    free(in->mem);
    free(in);
}

err arena_alloc(void **out, struct arena *in, size_t size, size_t alignment) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (size == 0) {
        status = CORE_ARGS;
        goto err;
    }

    uintptr_t current = (uintptr_t)in->mem + in->used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (in->used + padding + size > in->capacity) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    in->last_offset = in->used;
    in->used += padding + size;
    *out = (void *)aligned;
    memset(*out, 0, size);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Allocating to arena failed");
    return status;
}

err arena_reset(struct arena *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    in->used = 0;
    in->last_offset = 0;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Resetting arena failed");
    return status;
}

err arena_remove_last(struct arena *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    in->used = in->last_offset;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Freeing last in arena failed");
    return status;
}
