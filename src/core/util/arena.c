#include "core/util/arena.h"
#include "core/util/error.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int arena_init(struct arena *out, size_t capacity) {
    if (!out)
        return CORE_NULLPTR;

    out->mem = malloc(capacity);
    if (!out->mem)
        return CORE_OUT_OF_MEMORY;

    out->capacity = capacity;
    out->used = 0;
    out->last_offset = 0;
    return CORE_SUCCESS;
}

void arena_destroy(struct arena *in) {
    if (!in || !in->mem)
        return;

    free(in->mem);
    free(in);
}

int arena_alloc(void **out, struct arena *in, size_t size, size_t alignment) {
    if (!in)
        return CORE_NULLPTR;

    if (size == 0)
        return CORE_INVALID_ARGS;

    uintptr_t current = (uintptr_t)in->mem + in->used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (in->used + padding + size > in->capacity)
        return CORE_OUT_OF_MEMORY;

    in->last_offset = in->used;
    in->used += padding + size;
    *out = (void *)aligned;
    memset(*out, 0, size);
    return CORE_SUCCESS;
}

int arena_reset(struct arena *in) {
    if (!in)
        return CORE_NULLPTR;

    in->used = 0;
    in->last_offset = 0;
    return CORE_SUCCESS;
}

int arena_remove_last(struct arena *in) {
    if (!in)
        return CORE_NULLPTR;

    in->used = in->last_offset;
    return CORE_SUCCESS;
}
