#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include <stddef.h>

struct arena {
    void *mem;
    size_t capacity;
    size_t used;
    size_t last_offset;
};

int arena_init(struct arena *out, size_t capacity);
void arena_destroy(struct arena *in);
int arena_alloc(void **out, struct arena *in, size_t size, size_t alignment);
int arena_reset(struct arena *in);
int arena_remove_last(struct arena *in);

#endif // UTIL_ARENA_H
