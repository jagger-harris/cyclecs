#include "core/util/arena.h"
#include "core/util/logger.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct arena {
    void *memory;
    size_t capacity;
    size_t used;
    size_t last_offset;
};

err arena_new(arena **out, size_t capacity) {
    err err = CORE_SUCCESS;

    *out = malloc(sizeof(arena));
    if (!(*out)) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    (*out)->memory = malloc(capacity);
    if (!(*out)->memory) {
        free(out);
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    (*out)->capacity = capacity;
    (*out)->used = 0;
    (*out)->last_offset = 0;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new arena", err);
    return err;
}

err arena_delete(arena *in) {
    err err = CORE_SUCCESS;

    if (!in || !in->memory) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    free(in->memory);
    free(in);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete arena", err);
    return err;
}

err arena_alloc(void **out, arena *in, size_t size, size_t alignment) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (size == 0) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    uintptr_t current = (uintptr_t)in->memory + in->used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (in->used + padding + size > in->capacity) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    in->last_offset = in->used;
    in->used += padding + size;
    *out = (void *)aligned;
    memset(*out, 0, size);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to alloc to arena", err);
    return err;
}

err arena_reset(arena *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    in->used = 0;
    in->last_offset = 0;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to reset arena", err);
    return err;
}

err arena_remove_last(arena *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    in->used = in->last_offset;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to free last in arena", err);
    return err;
}
