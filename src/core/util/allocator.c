#include "core/util/allocator.h"
#include "core/util/error.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct allocator {
    allocator_alloc_fn alloc;
    allocator_free_fn free;
    void *ctx;
};

int allocator_create(struct allocator **out, allocator_alloc_fn alloc,
                     allocator_free_fn free, void *ctx) {
    if (!out)
        return CORE_NULLPTR;

    struct allocator *allocator = malloc(sizeof(struct allocator));
    if (!allocator)
        return CORE_OUT_OF_MEMORY;

    allocator->alloc = alloc;
    allocator->free = free;
    allocator->ctx = ctx;

    *out = allocator;
    return CORE_SUCCESS;
}

void allocator_destroy(struct allocator *in) {
    if (!in)
        return;

    free(in);
}

int allocator_alloc(void **out, struct allocator *in, size_t size,
                    size_t align) {
    return in->alloc(out, in->ctx, size, align);
}

void allocator_free(struct allocator *in, void *data) {
    in->free(data, in->ctx);
}
