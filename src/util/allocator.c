#include <cls/util/allocator.h>
#include <cls/util/error.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct cls_allocator {
    cls_allocator_alloc_fn alloc;
    cls_allocator_free_fn free;
    void *ctx;
};

int cls_allocator_create(struct cls_allocator **alloc,
                         cls_allocator_alloc_fn alloc_fn,
                         cls_allocator_free_fn free, void *ctx) {
    if (!alloc)
        return CLS_NULLPTR;

    struct cls_allocator *allocator = malloc(sizeof(struct cls_allocator));
    if (!allocator)
        return CLS_OUT_OF_MEMORY;

    allocator->alloc = alloc_fn;
    allocator->free = free;
    allocator->ctx = ctx;

    *alloc = allocator;
    return CLS_SUCCESS;
}

void cls_allocator_destroy(struct cls_allocator *alloc) {
    if (!alloc)
        return;

    free(alloc);
}

int cls_allocator_alloc(void **dest, struct cls_allocator *alloc, size_t size,
                        size_t align) {
    return alloc->alloc(dest, alloc->ctx, size, align);
}

void cls_allocator_free(struct cls_allocator *alloc, void *src) {
    alloc->free(src, alloc->ctx);
}
