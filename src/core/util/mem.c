#include "core/util/mem.h"
#include "core/util/error.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct mem {
    mem_alloc_fn alloc;
    mem_free_fn free;
    void *ctx;
};

int mem_create(struct mem **out, mem_alloc_fn alloc, mem_free_fn free,
               void *ctx) {
    if (!out)
        return CORE_NULLPTR;

    struct mem *mem = malloc(sizeof(struct mem));
    if (!mem)
        return CORE_OUT_OF_MEMORY;

    mem->alloc = alloc;
    mem->free = free;
    mem->ctx = ctx;

    *out = mem;
    return CORE_SUCCESS;
}

void mem_destroy(struct mem *in) {
    if (!in)
        return;

    free(in);
}

int mem_alloc(void **out, struct mem *in, size_t size, size_t align) {
    return in->alloc(out, in->ctx, size, align);
}

void mem_free(struct mem *in, void *data) {
    in->free(data, in->ctx);
}
