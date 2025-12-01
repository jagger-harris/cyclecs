#ifndef UTIL_MEM_H
#define UTIL_MEM_H

#include <stddef.h>

typedef int (*mem_alloc_fn)(void **out, void *ctx, size_t size, size_t align);
typedef void (*mem_free_fn)(void *in, void *ctx);
struct mem;

int mem_create(struct mem **out, mem_alloc_fn alloc, mem_free_fn free,
               void *ctx);
void mem_destroy(struct mem *in);
int mem_alloc(void **out, struct mem *in, size_t size, size_t align);
void mem_free(struct mem *in, void *data);

#endif // UTIL_MEM_H
