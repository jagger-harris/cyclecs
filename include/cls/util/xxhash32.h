#ifndef CLS_XXHASH32_H
#define CLS_XXHASH32_H

#include <cls/util/types.h>
#include <stddef.h>

int xxhash32(u32 *hash, const void *input, size_t length, u32 seed);

#endif // CLS_XXHASH32_H
