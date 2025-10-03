#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include <stddef.h>

struct arena {
    size_t size;
    size_t used;
    size_t last_offset;
    void *mem;
};

int arena_init(struct arena *out, size_t size);
void arena_destroy(struct arena *in);
int arena_alloc(void **out, struct arena *in, size_t size, size_t alignment);
int arena_clear(struct arena *in);
void arena_remove_last(struct arena *in);

#endif // UTIL_ARENA_H
