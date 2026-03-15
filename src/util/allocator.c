#include <cls/util/allocator.h>
#include <cls/util/error.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct allocator {
    allocator_alloc_fn alloc;
    allocator_free_fn free;
    void *ctx;
};

int allocator_create(struct allocator **alloc, allocator_alloc_fn alloc_fn,
                     allocator_free_fn free, void *ctx) {
    if (!alloc)
        return CLS_NULLPTR;

    struct allocator *allocator = malloc(sizeof(struct allocator));
    if (!allocator)
        return CLS_OUT_OF_MEMORY;

    allocator->alloc = alloc_fn;
    allocator->free = free;
    allocator->ctx = ctx;

    *alloc = allocator;
    return CLS_SUCCESS;
}

void allocator_destroy(struct allocator *alloc) {
    if (!alloc)
        return;

    free(alloc);
}

int allocator_alloc(void **dest, struct allocator *alloc, size_t size,
                    size_t align) {
    return alloc->alloc(dest, alloc->ctx, size, align);
}

void allocator_free(struct allocator *alloc, void *src) {
    alloc->free(src, alloc->ctx);
}
