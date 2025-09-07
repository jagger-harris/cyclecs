#ifndef UTIL_XXHASH64_H
#define UTIL_XXHASH64_H

#include "core/util/types.h"
#include <stddef.h>

u64 xxhash64(const void *input, size_t length, u64 seed);

#endif // UTIL_XXHASH64_H
