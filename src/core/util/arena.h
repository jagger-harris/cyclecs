#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include <stddef.h>

typedef struct arena arena;

int arena_new(arena **out, size_t capacity);
int arena_delete(arena *in);
int arena_alloc(void **out, arena *in, size_t size, size_t alignment);
int arena_reset(arena *in);
int arena_free_last(arena *in);

#endif /* UTIL_ARENA_H */
