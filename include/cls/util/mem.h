#ifndef CLS_MEM_H
#define CLS_MEM_H

#include <stddef.h>

typedef int (*cls_mem_alloc_fn)(void **dest, void *ctx, size_t size,
                                size_t align);
typedef void (*cls_mem_free_fn)(void *src, void *ctx);
struct cls_mem;

int cls_mem_create(struct cls_mem **alloc, cls_mem_alloc_fn alloc_fn,
                   cls_mem_free_fn free, void *ctx);
void cls_mem_destroy(struct cls_mem *alloc);
int cls_mem_alloc(void **dest, struct cls_mem *alloc, size_t size,
                  size_t align);
void cls_mem_free(struct cls_mem *alloc, void *src);

#endif // CLS_MEM_H
