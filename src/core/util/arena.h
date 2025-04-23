#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include "core/util/err.h"
#include <stddef.h>

typedef struct arena arena;

err arena_new(arena **out, size_t capacity);
err arena_delete(arena *in);
err arena_alloc(void **out, arena *in, size_t size, size_t alignment);
err arena_reset(arena *in);
err arena_remove_last(arena *in);

#endif /* UTIL_ARENA_H */
