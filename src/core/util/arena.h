#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include "core/util/err.h"
#include <stddef.h>

struct arena {
    void *mem;
    size_t capacity;
    size_t used;
    size_t last_offset;
};

err arena_init(struct arena *out, size_t capacity);
void arena_destroy(struct arena *in);
err arena_alloc(void **out, struct arena *in, size_t size, size_t alignment);
err arena_reset(struct arena *in);
err arena_remove_last(struct arena *in);

#endif // UTIL_ARENA_H
