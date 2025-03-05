#ifndef RSV_HTABLE_H
#define RSV_HTABLE_H

#include "../rsv.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RSV_HTABLE_MIN_CAPACITY 16
#define RSV_HTABLE_MAX_LOAD_FACTOR 0.75
#define RSV_HTABLE_MIN_LOAD_FACTOR 0.25
#define RSV_HTABLE_GROW_FACTOR 2
#define RSV_HTABLE_SHRINK_FACTOR 0.5

#define RSV_HTABLE_HEADER_FROM_ENTRIES(entries)                                \
  ((RSV_HTABLE_HEADER *)((uint8_t *)(entries) -                                \
                         offsetof(RSV_HTABLE_HEADER, entries)))

typedef uint32_t (*rsv_htable_hash_fn)(const void *key, size_t key_size);
typedef bool (*rsv_htable_compare_fn)(const void *key_a, const void *key_b,
                                      size_t key_size);

RSVDEF uint32_t rsv_htable_hash(const void *key, size_t key_size) {
  const uint8_t *bytes = key;
  uint32_t hash = 2166136261u;

  for (size_t i = 0; i < key_size; ++i) {
    hash ^= (uint8_t)bytes[i];
    hash *= 16777619;
  }

  return hash;
}

#endif // RSV_HTABLE_H

// ****************************************************************************
//
// GENERIC TYPES
//
// ****************************************************************************

#ifndef RSV_HTABLE_KEY_TYPE
#error "RSV_HTABLE_KEY_TYPE must be defined"
#endif
#ifndef RSV_HTABLE_VALUE_TYPE
#error "RSV_HTABLE_VALUE_TYPE must be defined"
#endif

#define RSV_HTABLE_ENTRY                                                       \
  RSV_CONCAT(rsv_htable_,                                                      \
             RSV_CONCAT(RSV_HTABLE_KEY_TYPE,                                   \
                        RSV_CONCAT(_, RSV_CONCAT(RSV_HTABLE_VALUE_TYPE,        \
                                                 RSV_CONCAT(_, entry)))))
#define RSV_HTABLE_HEADER                                                      \
  RSV_CONCAT(rsv_htable_, RSV_CONCAT(RSV_HTABLE_KEY_TYPE,                      \
                                     RSV_CONCAT(_, RSV_HTABLE_VALUE_TYPE)))
#define RSV_HTABLE_FN(suffix)                                                  \
  RSV_CONCAT(rsv_htable_##suffix,                                              \
             RSV_CONCAT(_, RSV_CONCAT(RSV_HTABLE_KEY_TYPE,                     \
                                      RSV_CONCAT(_, RSV_HTABLE_VALUE_TYPE))))

typedef struct RSV_HTABLE_ENTRY RSV_HTABLE_ENTRY;
struct RSV_HTABLE_ENTRY {
  RSV_HTABLE_KEY_TYPE key;
  RSV_HTABLE_VALUE_TYPE value;
};

typedef struct RSV_HTABLE_HEADER RSV_HTABLE_HEADER;
struct RSV_HTABLE_HEADER {
  size_t capacity;
  size_t length;
  uint8_t *metadata;
  RSV_HTABLE_ENTRY entries[];
};

// ****************************************************************************
//
// DEFINITIONS
//
// ****************************************************************************

/**
 * @brief Creates a new hash table with a given initial capacity.
 *
 * @param dest A pointer to store the created hash table.
 * @param start_capacity The initial capacity of the table.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * rsv_htable_int_int_entry* table = NULL;
 * rsv_htable_new_int_int(&table, 16);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(new)(RSV_HTABLE_ENTRY **dest,
                                          size_t start_capacity);

/**
 * @brief Deletes a hash table and frees allocated memory.
 *
 * @param entries_ptr A pointer to the hash table.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * rsv_htable_delete_int_int(&table);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(delete)(RSV_HTABLE_ENTRY **entries_ptr);

/**
 * @brief Gets the capacity of the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param dest A pointer to store the capacity.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * size_t capacity;
 * rsv_htable_capacity_int_int(&table, &capacity);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(capacity)(RSV_HTABLE_ENTRY **entries_ptr,
                                               size_t *dest);

/**
 * @brief Gets the capacity of the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param dest A pointer to store the capacity.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * size_t capacity;
 * rsv_htable_capacity_int_int(&table, &capacity);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(length)(RSV_HTABLE_ENTRY **entries_ptr,
                                             size_t *dest);

/**
 * @brief Gets the number of elements in the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param dest A pointer to store the length.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * size_t length;
 * rsv_htable_length_int_int(&table, &length);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(resize)(RSV_HTABLE_ENTRY **entries_ptr,
                                             size_t new_capacity);

/**
 * @brief Inserts a key-value pair into the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param key The key to insert.
 * @param value The value associated with the key.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * rsv_htable_insert_int_int(&table, 10, 100);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(insert)(RSV_HTABLE_ENTRY **entries_ptr,
                                             RSV_HTABLE_KEY_TYPE key,
                                             RSV_HTABLE_VALUE_TYPE value);

/**
 * @brief Removes a key from the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param key The key to remove.
 * @param dest A pointer to store whether the key was removed.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * bool removed;
 * rsv_htable_remove_int_int(&table, 10, &removed);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(remove)(RSV_HTABLE_ENTRY **entries_ptr,
                                             const RSV_HTABLE_KEY_TYPE key,
                                             bool *dest);

/**
 * @brief Finds a value by its key in the hash table.
 *
 * @param entries_ptr A pointer to the hash table.
 * @param key The key to look up.
 * @param dest A pointer to store the found value.
 * @return rsv_status Status indicating success or failure.
 *
 * @example
 * int value;
 * rsv_htable_find_int_int(&table, 10, &value);
 */
RSVDEF enum rsv_status RSV_HTABLE_FN(find)(RSV_HTABLE_ENTRY **entries_ptr,
                                           const RSV_HTABLE_KEY_TYPE key,
                                           RSV_HTABLE_VALUE_TYPE *dest);

// ****************************************************************************
//
// IMPLEMENTATIONS
//
// ****************************************************************************

#ifdef RSV_HTABLE_IMPLEMENTATION

RSVDEF enum rsv_status RSV_HTABLE_FN(new)(RSV_HTABLE_ENTRY **dest,
                                          size_t start_capacity) {
  if (!dest || 1 > start_capacity) {
    return RSV_STATUS_INVALID_VAR;
  }

  size_t capacity = rsv_get_power_of_2(start_capacity);

  if (capacity < RSV_HTABLE_MIN_CAPACITY) {
    capacity = RSV_HTABLE_MIN_CAPACITY;
  }

  RSV_HTABLE_HEADER *header =
      malloc(sizeof(RSV_HTABLE_HEADER) + capacity * sizeof(RSV_HTABLE_ENTRY));
  if (!header) {
    return RSV_STATUS_CANNOT_ALLOC_MEMORY;
  }

  header->capacity = capacity;
  header->length = 0;
  header->metadata = malloc(capacity * sizeof(uint8_t));
  if (!header->metadata) {
    free(header);
    return RSV_STATUS_CANNOT_ALLOC_MEMORY;
  }

  for (size_t i = 0; i < capacity; ++i) {
    memset(&header->metadata[i], 0, sizeof(uint8_t));
  }

  *dest = header->entries;

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(delete)(RSV_HTABLE_ENTRY **entries_ptr) {
  if (!entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);
  *entries_ptr = NULL;
  free(header->metadata);
  free(header);

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(capacity)(RSV_HTABLE_ENTRY **entries_ptr,
                                               size_t *dest) {
  if (!dest || !entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);
  *dest = header->capacity;

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(length)(RSV_HTABLE_ENTRY **entries_ptr,
                                             size_t *dest) {
  if (!dest || !entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);
  *dest = header->length;

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(resize)(RSV_HTABLE_ENTRY **entries_ptr,
                                             size_t new_capacity) {
  if (!entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);

  if (header->capacity < RSV_HTABLE_MIN_CAPACITY) {
    return RSV_STATUS_SUCCESS;
  }

  RSV_HTABLE_HEADER *new_header = malloc(
      sizeof(RSV_HTABLE_HEADER) + new_capacity * sizeof(RSV_HTABLE_ENTRY));
  if (!new_header) {
    return RSV_STATUS_CANNOT_ALLOC_MEMORY;
  }

  new_header->capacity = new_capacity;
  new_header->length = 0;
  new_header->metadata = malloc(new_capacity * sizeof(uint8_t));
  if (!new_header->metadata) {
    free(new_header);
    return RSV_STATUS_CANNOT_ALLOC_MEMORY;
  }

  for (size_t i = 0; i < new_capacity; ++i) {
    memset(&new_header->metadata[i], 0, sizeof(uint8_t));
  }

  RSV_HTABLE_ENTRY *new_entries = new_header->entries;

  for (size_t i = 0; i < header->capacity; ++i) {
    if (header->metadata[i] != 0) {
      rsv_status status = RSV_HTABLE_FN(insert)(
          &new_entries, header->entries[i].key, header->entries[i].value);

      if (status != RSV_STATUS_SUCCESS) {
        free(new_header->metadata);
        free(new_header);
        return status;
      }
    }
  }

  *entries_ptr = new_entries;
  free(header->metadata);
  free(header);

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(insert)(RSV_HTABLE_ENTRY **entries_ptr,
                                             RSV_HTABLE_KEY_TYPE key,
                                             RSV_HTABLE_VALUE_TYPE value) {
  if (!entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);

  if ((float)header->length / header->capacity > RSV_HTABLE_MAX_LOAD_FACTOR) {
    rsv_status status = RSV_HTABLE_FN(resize)(
        entries_ptr, header->capacity * RSV_HTABLE_GROW_FACTOR);

    if (status != RSV_STATUS_SUCCESS) {
      return status;
    }

    entries = *entries_ptr;
    header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);
  }

  uint32_t hash = rsv_htable_hash(&key, sizeof(key));
  uint8_t fingerprint = (hash & 0x7F) | 0x80;
  uint32_t index = hash & (header->capacity - 1);

  for (size_t i = 0; i < header->capacity; ++i) {
    uint32_t probe = (index + i) & (header->capacity - 1);
    uint8_t probe_metadata = header->metadata[probe];

    if (probe_metadata == 0) {
      header->length++;
      header->entries[probe].key = key;
      header->entries[probe].value = value;
      header->metadata[probe] = fingerprint;

      return RSV_STATUS_SUCCESS;
    }
  }

  return RSV_STATUS_FAILURE;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(remove)(RSV_HTABLE_ENTRY **entries_ptr,
                                             const RSV_HTABLE_KEY_TYPE key,
                                             bool *dest) {
  if (!entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);

  uint32_t hash = rsv_htable_hash(&key, sizeof(key));
  uint8_t hash_metadata = (hash & 0x7F) | 0x80;
  uint32_t index = hash & (header->capacity - 1);

  for (size_t i = 0; i < header->capacity; ++i) {
    uint32_t probe = (index + i) & (header->capacity - 1);
    uint8_t probe_metadata = header->metadata[probe];

    if (probe_metadata == 0) {
      if (dest) {
        *dest = false;
      }

      return RSV_STATUS_SUCCESS;
    }

    if (header->metadata[probe] == hash_metadata &&
        memcmp(&header->entries[probe].key, &key, sizeof(key)) == 0) {
      header->length--;
      header->metadata[probe] = 0;

      if ((float)header->length / header->capacity <
          RSV_HTABLE_MIN_LOAD_FACTOR) {
        rsv_status status = RSV_HTABLE_FN(resize)(
            entries_ptr, header->capacity * RSV_HTABLE_SHRINK_FACTOR);

        if (status != RSV_STATUS_SUCCESS) {
          return status;
        }

        entries = *entries_ptr;
        header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);
      }

      if (dest) {
        *dest = true;
      }

      return RSV_STATUS_SUCCESS;
    }
  }

  if (dest) {
    *dest = false;
  }

  return RSV_STATUS_SUCCESS;
}

RSVDEF enum rsv_status RSV_HTABLE_FN(find)(RSV_HTABLE_ENTRY **entries_ptr,
                                           const RSV_HTABLE_KEY_TYPE key,
                                           RSV_HTABLE_VALUE_TYPE *dest) {
  if (!dest || !entries_ptr || !*entries_ptr) {
    return RSV_STATUS_INVALID_NULLPTR;
  }

  RSV_HTABLE_ENTRY *entries = *entries_ptr;
  RSV_HTABLE_HEADER *header = RSV_HTABLE_HEADER_FROM_ENTRIES(entries);

  uint32_t hash = rsv_htable_hash(&key, sizeof(key));
  uint8_t fingerprint = (hash & 0x7F) | 0x80;
  uint32_t index = hash & (header->capacity - 1);

  for (size_t i = 0; i < header->capacity; ++i) {
    uint32_t probe = (index + i) & (header->capacity - 1);
    uint8_t probe_metadata = header->metadata[probe];

    if (probe_metadata == 0) {
      return RSV_STATUS_SUCCESS;
    }

    if (header->metadata[probe] == fingerprint &&
        memcmp(&header->entries[probe].key, &key, sizeof(key)) == 0) {
      *dest = header->entries[probe].value;

      return RSV_STATUS_SUCCESS;
    }
  }

  return RSV_STATUS_SUCCESS;
}

#endif

// ****************************************************************************
//
// UNDEFINE GENERIC TYPES
//
// ****************************************************************************

#undef RSV_HTABLE_KEY_TYPE
#undef RSV_HTABLE_VALUE_TYPE
#undef RSV_HTABLE_HEADER
#undef RSV_HTABLE_FN
