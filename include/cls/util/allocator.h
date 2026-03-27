#ifndef CLS_ALLOCATOR_H
#define CLS_ALLOCATOR_H

#include <stddef.h>

typedef int (*cls_allocator_alloc_fn)(void **dest, void *ctx, size_t size,
                                      size_t align);
typedef void (*cls_allocator_free_fn)(void *src, void *ctx);
struct cls_allocator;

int cls_allocator_create(struct cls_allocator **alloc,
                         cls_allocator_alloc_fn alloc_fn,
                         cls_allocator_free_fn free, void *ctx);
void cls_allocator_destroy(struct cls_allocator *alloc);
int cls_allocator_alloc(void **dest, struct cls_allocator *alloc, size_t size,
                        size_t align);
void cls_allocator_free(struct cls_allocator *alloc, void *src);

#endif // CLS_ALLOCATOR_H
