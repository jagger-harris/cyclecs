#ifndef UTIL_ARENA_H
#define UTIL_ARENA_H

#include <stddef.h>

typedef size_t arena_marker;
struct arena;

int arena_create(struct arena **out, size_t size);
void arena_destroy(struct arena *in);
int arena_alloc(void **out, struct arena *in, size_t size, size_t align);
int arena_clear(struct arena *in);
int arena_marker_save(arena_marker *out, struct arena *in);
int arena_marker_restore(struct arena *in, arena_marker *marker);

#endif // UTIL_ARENA_H
