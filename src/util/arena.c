#include <cls/util/arena.h>
#include <cls/util/error.h>
#include <cls/util/types.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct cls_arena {
    size_t size;
    size_t used;
    size_t last_offset;
    void *data;
};

static inline void *arena_data(struct cls_arena *a) {
    return (u8 *)a + sizeof(struct cls_arena);
}

int cls_arena_create(struct cls_arena **a, size_t size) {
    if (!a)
        return CLS_NULLPTR;

    if (size < 1)
        return CLS_INVALID_ARG;

    const size_t align = alignof(max_align_t);
    size_t total_size = sizeof(struct cls_arena) + size + (align - 1);

    struct cls_arena *arena = malloc(total_size);
    if (!arena)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)arena + sizeof(struct cls_arena);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);

    arena->data = (void *)aligned;
    arena->size = size;
    arena->used = 0;
    arena->last_offset = 0;

    *a = arena;
    return CLS_SUCCESS;
}

void cls_arena_destroy(struct cls_arena *a) {
    if (!a)
        return;

    free(a);
}

int cls_arena_alloc(void **dest, struct cls_arena *a, size_t size,
                    size_t align) {
    if (!dest || !a)
        return CLS_NULLPTR;

    if (size == 0 || (align & (align - 1)) != 0)
        return CLS_INVALID_ARG;

    uintptr_t current_offset = a->used;
    uintptr_t aligned_offset =
        (current_offset + align - 1) & ~(uintptr_t)(align - 1);

    if (aligned_offset + size > a->size)
        return CLS_OUT_OF_MEMORY;

    a->last_offset = a->used;
    a->used = aligned_offset + size;

    void *data = arena_data(a);
    *dest = (u8 *)data + aligned_offset;
    return CLS_SUCCESS;
}

int cls_arena_clear(struct cls_arena *a) {
    if (!a)
        return CLS_NULLPTR;

    a->used = 0;
    a->last_offset = 0;
    return CLS_SUCCESS;
}

int cls_arena_marker_save(cls_arena_marker *marker, struct cls_arena *a) {
    if (!marker || !a)
        return CLS_NULLPTR;

    *marker = a->used;
    return CLS_SUCCESS;
}

int cls_arena_marker_restore(struct cls_arena *a, cls_arena_marker *marker) {
    if (!a || !marker)
        return CLS_NULLPTR;

    a->used = *marker;
    return CLS_SUCCESS;
}
