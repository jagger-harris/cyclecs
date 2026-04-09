#include <cls/util/error.h>
#include <cls/util/mem.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct cls_mem {
    cls_mem_alloc_fn alloc;
    cls_mem_free_fn free;
    void *ctx;
};

int cls_mem_create(struct cls_mem **alloc, cls_mem_alloc_fn alloc_fn,
                   cls_mem_free_fn free, void *ctx) {
    if (!alloc)
        return CLS_NULLPTR;

    struct cls_mem *allocator = malloc(sizeof(struct cls_mem));
    if (!allocator)
        return CLS_OUT_OF_MEMORY;

    allocator->alloc = alloc_fn;
    allocator->free = free;
    allocator->ctx = ctx;

    *alloc = allocator;
    return CLS_SUCCESS;
}

void cls_mem_destroy(struct cls_mem *alloc) {
    if (!alloc)
        return;

    free(alloc);
}

int cls_mem_alloc(void **dest, struct cls_mem *alloc, size_t size,
                  size_t align) {
    return alloc->alloc(dest, alloc->ctx, size, align);
}

void cls_mem_free(struct cls_mem *alloc, void *src) {
    alloc->free(src, alloc->ctx);
}
