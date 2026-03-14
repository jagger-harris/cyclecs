#ifndef UTIL_XXHASH32_H
#define UTIL_XXHASH32_H

#include <cls/util/types.h>
#include <stddef.h>

int xxhash32(u32 *out, const void *input, size_t length, u32 seed);

#endif // UTIL_XXHASH32_H
