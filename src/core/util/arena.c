#include "core/util/arena.h"
#include "core/util/error.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int arena_init(struct arena *out, size_t size) {
    if (!out)
        return CORE_NULLPTR;

    *out =
        (struct arena){.mem = NULL, .size = size, .used = 0, .last_offset = 0};

    out->mem = malloc(size);
    if (!out->mem)
        return CORE_OUT_OF_MEMORY;

    return CORE_SUCCESS;
}

void arena_destroy(struct arena *in) {
    if (!in)
        return;

    if (in->mem) {
        free(in->mem);
        in->mem = NULL;
    }
}

int arena_alloc(void **out, struct arena *in, size_t size, size_t alignment) {
    if (!out || !in)
        return CORE_NULLPTR;

    if (size == 0 || (alignment & (alignment - 1)) != 0)
        return CORE_INVALID_ARG;

    uintptr_t current_offset = in->used;
    uintptr_t aligned_offset =
        (current_offset + alignment - 1) & ~(uintptr_t)(alignment - 1);

    if (aligned_offset + size > in->size)
        return CORE_OUT_OF_MEMORY;

    in->last_offset = in->used;
    in->used = aligned_offset + size;

    *out = (char *)in->mem + aligned_offset;
    memset(*out, 0, size);
    return CORE_SUCCESS;
}

int arena_clear(struct arena *in) {
    if (!in)
        return CORE_NULLPTR;

    in->used = 0;
    in->last_offset = 0;
    return CORE_SUCCESS;
}

void arena_remove_last(struct arena *in) {
    if (!in)
        return;

    in->used = in->last_offset;
}
