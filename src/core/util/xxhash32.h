#ifndef UTIL_XXHASH32_H
#define UTIL_XXHASH32_H

#include "core/util/types.h"
#include <stddef.h>

u32 xxhash32(const void *input, size_t length, u32 seed);

#endif // UTIL_XXHASH32_H
