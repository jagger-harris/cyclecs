#ifndef CLS_ALLOCATOR_H
#define CLS_ALLOCATOR_H

#include <stddef.h>

typedef int (*allocator_alloc_fn)(void **dest, void *ctx, size_t size,
                                  size_t align);
typedef void (*allocator_free_fn)(void *src, void *ctx);
struct allocator;

int allocator_create(struct allocator **alloc, allocator_alloc_fn alloc_fn,
                     allocator_free_fn free, void *ctx);
void allocator_destroy(struct allocator *alloc);
int allocator_alloc(void **dest, struct allocator *alloc, size_t size,
                    size_t align);
void allocator_free(struct allocator *alloc, void *src);

#endif // CLS_ALLOCATOR_H
