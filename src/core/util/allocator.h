#ifndef UTIL_ALLOCATOR_H
#define UTIL_ALLOCATOR_H

#include <stddef.h>

typedef int (*allocator_alloc_fn)(void **out, void *ctx, size_t size,
                                  size_t align);
typedef void (*allocator_free_fn)(void *in, void *ctx);
struct allocator;

int allocator_create(struct allocator **out, allocator_alloc_fn alloc,
                     allocator_free_fn free, void *ctx);
void allocator_destroy(struct allocator *in);
int allocator_alloc(void **out, struct allocator *in, size_t size,
                    size_t align);
void allocator_free(struct allocator *in, void *data);

#endif // UTIL_ALLOCATOR_H
