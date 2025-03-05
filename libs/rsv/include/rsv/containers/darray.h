#ifndef RSV_DARRAY_H
#define RSV_DARRAY_H

#include "../rsv.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define RSV_DARRAY_GROWTH_FACTOR 1.5

#define RSV_DARRAY_HEADER_FROM_ITEMS(items)                                    \
  ((RSV_DARRAY_HEADER *)((uint8_t *)(items) -                                  \
                         offsetof(RSV_DARRAY_HEADER, items)))

#endif // RSV_DARRAY_H

// ****************************************************************************
//
// GENERIC TYPES
//
// ****************************************************************************

#ifndef RSV_DARRAY_TYPE
#error "RSV_DARRAY_TYPE must be defined"
#endif

#define RSV_DARRAY_HEADER RSV_CONCAT(rsv_darray_, RSV_DARRAY_TYPE)
#define RSV_DARRAY_FN(suffix)                                                  \
  RSV_CONCAT(rsv_darray_##suffix, RSV_CONCAT(_, RSV_DARRAY_TYPE))

typedef struct RSV_DARRAY_HEADER RSV_DARRAY_HEADER;
struct RSV_DARRAY_HEADER {
  size_t capacity;
  size_t length;
  RSV_DARRAY_TYPE items[];
};

// ****************************************************************************
//
// DEFINITIONS
//
// ****************************************************************************

/**
 * @brief Creates a new dynamic array.
 *
 * @param[out] dest Pointer to store the new array.
 * @param[in] start_capacity Initial capacity of the array.
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(new)(RSV_DARRAY_TYPE **dest,
                                         size_t start_capacity);

/**
 * @brief Deletes a dynamic array.
 *
 * @param[in,out] items_ptr Pointer to the dynamic array.
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(delete)(RSV_DARRAY_TYPE **items_ptr);

/**
 * @brief Retrieves the capacity of the dynamic array.
 *
 * @param[in] items_ptr Pointer to the dynamic array.
 * @param[out] dest Pointer to store the capacity.
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(capacity)(RSV_DARRAY_TYPE **items_ptr,
                                              size_t *dest);

/**
 * @brief Retrieves the length (number of elements) of the dynamic array.
 *
 * @param[in] items_ptr Pointer to the dynamic array.
 * @param[out] dest Pointer to store the length.
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(length)(RSV_DARRAY_TYPE **items_ptr,
                                            size_t *dest);

/**
 * @brief Pushes an element onto the dynamic array.
 *
 * @param[in,out] items_ptr Pointer to the dynamic array.
 * @param[in] value Value to be added.
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(push)(RSV_DARRAY_TYPE **items_ptr,
                                          RSV_DARRAY_TYPE value);

/**
 * @brief Pops an element from the dynamic array.
 *
 * @param[in,out] items_ptr Pointer to the dynamic array.
 * @param[out] dest Pointer to store the popped value (optional).
 *
 * @return Status code indicating success or failure.
 */
RSVDEF enum rsv_error RSV_DARRAY_FN(pop)(RSV_DARRAY_TYPE **items_ptr,
                                         RSV_DARRAY_TYPE *dest);

// ****************************************************************************
//
// IMPLEMENTATIONS
//
// ****************************************************************************

#ifdef RSV_DARRAY_IMPLEMENTATION

RSVDEF enum rsv_error RSV_DARRAY_FN(new)(RSV_DARRAY_TYPE **dest,
                                         size_t start_capacity) {
  if (!dest || 1 > start_capacity) {
    return rsv_error_INVALID_VAR;
  }

  RSV_DARRAY_HEADER *header = malloc(sizeof(RSV_DARRAY_HEADER) +
                                     start_capacity * sizeof(RSV_DARRAY_TYPE));
  if (!header) {
    return rsv_error_CANNOT_ALLOC_MEMORY;
  }

  header->capacity = start_capacity;
  header->length = 0;
  *dest = header->items;

  return rsv_error_SUCCESS;
}

RSVDEF enum rsv_error RSV_DARRAY_FN(delete)(RSV_DARRAY_TYPE **items_ptr) {
  if (!items_ptr || !*items_ptr) {
    return rsv_error_INVALID_NULLPTR;
  }

  RSV_DARRAY_TYPE *items = *items_ptr;
  RSV_DARRAY_HEADER *header = RSV_DARRAY_HEADER_FROM_ITEMS(items);
  free(header);
  *items_ptr = NULL;

  return rsv_error_SUCCESS;
}

RSVDEF enum rsv_error RSV_DARRAY_FN(capacity)(RSV_DARRAY_TYPE **items_ptr,
                                              size_t *dest) {
  if (!items_ptr || !*items_ptr || !dest) {
    return rsv_error_INVALID_NULLPTR;
  }

  RSV_DARRAY_TYPE *items = *items_ptr;
  RSV_DARRAY_HEADER *header = RSV_DARRAY_HEADER_FROM_ITEMS(items);
  *dest = header->capacity;

  return rsv_error_SUCCESS;
}

RSVDEF enum rsv_error RSV_DARRAY_FN(length)(RSV_DARRAY_TYPE **items_ptr,
                                            size_t *dest) {
  if (!items_ptr || !*items_ptr || !dest) {
    return rsv_error_INVALID_NULLPTR;
  }

  RSV_DARRAY_TYPE *items = *items_ptr;
  RSV_DARRAY_HEADER *header = RSV_DARRAY_HEADER_FROM_ITEMS(items);
  *dest = header->length;

  return rsv_error_SUCCESS;
}

RSVDEF enum rsv_error RSV_DARRAY_FN(push)(RSV_DARRAY_TYPE **items_ptr,
                                          RSV_DARRAY_TYPE value) {
  if (!items_ptr || !*items_ptr) {
    return rsv_error_INVALID_NULLPTR;
  }

  RSV_DARRAY_TYPE *items = *items_ptr;
  RSV_DARRAY_HEADER *header = RSV_DARRAY_HEADER_FROM_ITEMS(items);

  if (header->length >= header->capacity) {
    size_t new_capacity = header->capacity * RSV_DARRAY_GROWTH_FACTOR;
    void *temp = realloc(header, sizeof(RSV_DARRAY_HEADER) +
                                     sizeof(RSV_DARRAY_TYPE) * new_capacity);

    if (!temp) {
      return rsv_error_CANNOT_ALLOC_MEMORY;
    }

    header = temp;
    header->capacity = new_capacity;
    *items_ptr = header->items;
  }

  (*items_ptr)[header->length++] = value;

  return rsv_error_SUCCESS;
}

RSVDEF enum rsv_error RSV_DARRAY_FN(pop)(RSV_DARRAY_TYPE **items_ptr,
                                         RSV_DARRAY_TYPE *dest) {
  if (!items_ptr || !*items_ptr) {
    return rsv_error_INVALID_NULLPTR;
  }

  RSV_DARRAY_TYPE *items = *items_ptr;
  RSV_DARRAY_HEADER *header = RSV_DARRAY_HEADER_FROM_ITEMS(items);

  if (header->length == 0) {
    return rsv_error_POP_FROM_EMPTY;
  }

  if (dest) {
    *dest = (*items_ptr)[--header->length];
  }

  return rsv_error_SUCCESS;
}

#endif

// ****************************************************************************
//
// UNDEFINE GENERIC TYPES
//
// ****************************************************************************

#undef RSV_DARRAY_TYPE
#undef RSV_DARRAY_HEADER
#undef RSV_DARRAY_FN
