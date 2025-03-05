#ifndef RSV_LIB_H
#define RSV_LIB_H

#include <stdbool.h>
#include <stddef.h>

#define RSV_CONCAT_TEMP(prefix, suffix) prefix##suffix
#define RSV_CONCAT(prefix, suffix) RSV_CONCAT_TEMP(prefix, suffix)

#ifndef RSVDEF
#define RSVDEF static inline
#endif

enum rsv_error {
  RSV_ERROR_SUCCESS,
  RSV_ERROR_FAILURE,
  RSV_ERROR_INVALID_NULLPTR,
  RSV_ERROR_CANNOT_ALLOC_MEMORY,
  RSV_ERROR_INVALID_VAR,
  RSV_ERROR_POP_FROM_EMPTY
};

RSVDEF bool rsv_is_power_of_2(size_t n) { return (n & (n - 1)) == 0; }

RSVDEF size_t rsv_get_power_of_2(size_t n) {
  if (rsv_is_power_of_2(n)) {
    return n;
  }

  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  n++;

  return n;
}

#endif // RSV_LIB_H
