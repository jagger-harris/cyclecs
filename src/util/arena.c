#include <cls/util/arena.h>
#include <cls/util/error.h>
#include <cls/util/types.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct arena {
    size_t size;
    size_t used;
    size_t last_offset;
    void *data;
};

static inline void *arena_data(struct arena *in) {
    return (u8 *)in + sizeof(struct arena);
}

int arena_create(struct arena **out, size_t size) {
    if (!out)
        return CLS_NULLPTR;

    if (size == 0)
        return CLS_INVALID_ARG;

    const size_t align = alignof(max_align_t);
    size_t total_size = sizeof(struct arena) + size + (align - 1);

    struct arena *arena = malloc(total_size);
    if (!arena)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)arena + sizeof(struct arena);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);

    arena->data = (void *)aligned;
    arena->size = size;
    arena->used = 0;
    arena->last_offset = 0;

    *out = arena;
    return CLS_SUCCESS;
}

void arena_destroy(struct arena *in) {
    if (!in)
        return;

    free(in);
}

int arena_alloc(void **out, struct arena *in, size_t size, size_t align) {
    if (!out || !in)
        return CLS_NULLPTR;

    if (size == 0 || (align & (align - 1)) != 0)
        return CLS_INVALID_ARG;

    uintptr_t current_offset = in->used;
    uintptr_t aligned_offset =
        (current_offset + align - 1) & ~(uintptr_t)(align - 1);

    if (aligned_offset + size > in->size)
        return CLS_OUT_OF_MEMORY;

    in->last_offset = in->used;
    in->used = aligned_offset + size;

    void *data = arena_data(in);
    *out = (u8 *)data + aligned_offset;
    return CLS_SUCCESS;
}

int arena_clear(struct arena *in) {
    if (!in)
        return CLS_NULLPTR;

    in->used = 0;
    in->last_offset = 0;
    return CLS_SUCCESS;
}

int arena_marker_save(arena_marker *out, struct arena *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->used;
    return CLS_SUCCESS;
}

int arena_marker_restore(struct arena *in, arena_marker *marker) {
    if (!in || !marker)
        return CLS_NULLPTR;

    in->used = *marker;
    return CLS_SUCCESS;
}
