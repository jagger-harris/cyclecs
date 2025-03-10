#include "arena.h"
#include "error.h"
#include "logger.h"
#include <stdint.h>
#include <stdlib.h>

struct arena {
    void *memory;
    size_t capacity;
    size_t used;
    size_t last_offset;
};

int arena_new(arena **out, size_t capacity) {
    int error = CORE_SUCCESS;

    *out = malloc(sizeof(arena));
    if (!out) {
        error = CORE_CANNOT_ALLOC_MEMORY;
        goto error;
    }

    (*out)->memory = malloc(capacity);
    if (!(*out)->memory) {
        free(out);
        error = CORE_CANNOT_ALLOC_MEMORY;
        goto error;
    }

    (*out)->capacity = capacity;
    (*out)->used = 0;
    (*out)->last_offset = 0;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to make new arena", error);
    return error;
}

int arena_delete(arena *in) {
    int error = CORE_SUCCESS;

    if (!in || !in->memory) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    free(in->memory);
    free(in);
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to delete arena", error);
    return error;
}

int arena_alloc(void **out, arena *in, size_t size, size_t alignment) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    if (size == 0) {
        error = CORE_INVALID_VAR;
        goto error;
    }

    uintptr_t current = (uintptr_t)in->memory + in->used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (in->used + padding + size > in->capacity) {
        error = CORE_OUT_OF_MEMORY;
        goto error;
    }

    in->last_offset = in->used;
    in->used += padding + size;
    *out = (void *)aligned;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to alloc to arena", error);
    return error;
}

int arena_reset(arena *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    in->used = 0;
    in->last_offset = 0;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to reset arena", error);
    return error;
}

int arena_free_last(arena *in) {
    int error = CORE_SUCCESS;

    if (!in) {
        error = CORE_INVALID_NULLPTR;
        goto error;
    }

    in->used = in->last_offset;
    return error;

error:
    logger_log(LOGGER_ERROR, "Failed to free last in arena", error);
    return error;
}
