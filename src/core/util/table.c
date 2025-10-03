#include "core/util/table.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/xxhash32.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MIN_CAPACITY 16
#define TABLE_MIN_LOAD_FACTOR 0.25
#define TABLE_MAX_LOAD_FACTOR 0.75
#define TABLE_GROWTH_FACTOR 2
#define TABLE_SHRINK_FACTOR 0.5
#define SLOT_METADATA_SIZE 8

static size_t table_calculate_slot_size(size_t key_size, size_t value_size) {
    return SLOT_METADATA_SIZE + key_size + value_size;
}

static int table_get_slot(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = (u8 *)in->slots + index * in->slot_size;
    return CORE_SUCCESS;
}

static int table_get_metadata(u8 **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    *out = (u8 *)slot;
    return CORE_SUCCESS;
}

static int table_get_key(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    *out = (u8 *)slot + SLOT_METADATA_SIZE;
    return CORE_SUCCESS;
}

static int table_get_value(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    *out = (u8 *)slot + SLOT_METADATA_SIZE + in->key_size;
    return CORE_SUCCESS;
}

static int table_insert_no_resize(struct table *in, const void *key,
                                  const void *value) {
    if (!in)
        return CORE_NULLPTR;

    u32 hash = xxhash32(key, in->key_size, 0);
    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (index + i) & (in->capacity - 1);
        u8 *probe_metadata = NULL;
        int error = table_get_metadata(&probe_metadata, in, probe);
        if (error)
            return error;

        if (*probe_metadata == 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, in, probe);
            if (error)
                return error;

            error = table_get_value(&value_ptr, in, probe);
            if (error)
                return error;

            memcpy(key_ptr, key, in->key_size);
            memcpy(value_ptr, value, in->value_size);

            *probe_metadata = fingerprint;
            in->length++;
            return CORE_SUCCESS;
        }
    }

    return CORE_OUT_OF_MEMORY;
}

static int table_resize(struct table *in, size_t new_capacity) {
    if (!in)
        return CORE_NULLPTR;

    struct table new_table = {0};
    int error = CORE_SUCCESS;

    new_table.capacity = new_capacity;
    new_table.length = 0;
    new_table.key_size = in->key_size;
    new_table.value_size = in->value_size;
    new_table.slot_size = in->slot_size;

    new_table.slots = malloc(new_capacity * new_table.slot_size);
    if (!new_table.slots) {
        error = CORE_OUT_OF_MEMORY;
        goto cleanup;
    }

    for (size_t i = 0; i < new_capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, &new_table, i);
        if (error)
            goto cleanup;
        *metadata = 0;
    }

    for (size_t i = 0; i < in->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, in, i);
        if (error)
            goto cleanup;

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, in, i);
            if (error)
                goto cleanup;

            error = table_get_value(&value_ptr, in, i);
            if (error)
                goto cleanup;

            error = table_insert_no_resize(&new_table, key_ptr, value_ptr);
            if (error)
                goto cleanup;
        }
    }

    free(in->slots);
    in->slots = new_table.slots;
    in->capacity = new_table.capacity;
    return CORE_SUCCESS;

cleanup:
    if (new_table.slots)
        free(new_table.slots);

    return error;
}

int table_init(struct table *out, size_t start_capacity, size_t key_size,
               size_t value_size) {
    if (!out)
        return CORE_NULLPTR;

    size_t slot_size = table_calculate_slot_size(key_size, value_size);

    *out = (struct table){.capacity = start_capacity,
                          .length = 0,
                          .key_size = key_size,
                          .value_size = value_size,
                          .slot_size = slot_size,
                          .slots = NULL};

    out->slots = malloc(start_capacity * slot_size);
    if (!out->slots) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, CORE_OUT_OF_MEMORY, "%s",
                         "Allocating table slots failed");
        return CORE_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < out->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, out, i);
        if (error) {
            free(out->slots);
            out->slots = NULL;
            return error;
        }

        *metadata = 0;
    }

    return CORE_SUCCESS;
}

void table_destroy(struct table *in) {
    if (!in)
        return;

    if (in->slots) {
        free(in->slots);
        in->slots = NULL;
    }
}

int table_insert(struct table *in, const void *key, const void *value) {
    if (!in || !key || !value)
        return CORE_NULLPTR;

    if ((double)in->length / (double)in->capacity > TABLE_MAX_LOAD_FACTOR) {
        int error = table_resize(in, in->capacity * TABLE_GROWTH_FACTOR);
        if (error)
            return error;
    }

    return table_insert_no_resize(in, key, value);
}

int table_remove(void *out, struct table *in, const void *key) {
    if (!in || !key)
        return CORE_NULLPTR;

    u32 hash = xxhash32(key, in->key_size, 0);
    u8 hash_metadata = (hash & 0x7f) | 0x80;
    u32 index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (index + i) & (in->capacity - 1);
        u8 *probe_metadata = NULL;
        int error = table_get_metadata(&probe_metadata, in, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CORE_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = table_get_key(&key_ptr, in, probe);
        if (error)
            return error;

        error = table_get_value(&value_ptr, in, probe);
        if (error)
            return error;

        if (*probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            in->length--;
            *probe_metadata = 0;

            if ((double)in->length / (double)in->capacity <
                TABLE_MIN_LOAD_FACTOR) {
                error = table_resize(
                    in, (size_t)((double)in->capacity * TABLE_SHRINK_FACTOR));
                if (error)
                    return error;
            }

            if (out)
                memcpy(out, value_ptr, in->value_size);

            return CORE_SUCCESS;
        }
    }

    return CORE_SUCCESS;
}

int table_find(void **out, const struct table *in, const void *key) {
    if (!out || !in || !key)
        return CORE_NULLPTR;

    u32 hash = xxhash32(key, in->key_size, 0);
    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (index + i) & (in->capacity - 1);
        u8 *probe_metadata = NULL;
        int error = table_get_metadata(&probe_metadata, in, probe);
        if (error)
            return error;

        if (*probe_metadata == 0) {
            *out = NULL;
            return CORE_SUCCESS;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = table_get_key(&key_ptr, in, probe);
        if (error)
            return error;

        error = table_get_value(&value_ptr, in, probe);
        if (error)
            return error;

        if (*probe_metadata == fingerprint &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            *out = value_ptr;
            return CORE_SUCCESS;
        }
    }

    *out = NULL;
    return CORE_SUCCESS;
}

int table_find_cpy(void *out, const struct table *in, const void *key) {
    void *value = NULL;
    int error = table_find(&value, in, key);
    if (error)
        return error;

    if (value != NULL)
        memcpy(out, value, in->value_size);

    return CORE_SUCCESS;
}

int table_iterator_init(struct table_iterator *iter,
                        const struct table *table) {
    if (!iter || !table)
        return CORE_NULLPTR;

    iter->table = table;
    iter->current_index = 0;
    iter->key = NULL;
    iter->value = NULL;

    return CORE_SUCCESS;
}

bool table_iterator_next(struct table_iterator *iter) {
    if (!iter || !iter->table)
        return false;

    while (iter->current_index < iter->table->capacity) {
        u8 *metadata = NULL;
        int error =
            table_get_metadata(&metadata, iter->table, iter->current_index);
        if (error)
            return false;

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, iter->table, iter->current_index);
            if (error)
                return false;

            error =
                table_get_value(&value_ptr, iter->table, iter->current_index);
            if (error)
                return false;

            iter->key = key_ptr;
            iter->value = value_ptr;
            iter->current_index++;
            return true;
        }
        iter->current_index++;
    }

    iter->key = NULL;
    iter->value = NULL;
    return false;
}

int table_iterator_reset(struct table_iterator *iter) {
    if (!iter)
        return CORE_NULLPTR;

    iter->current_index = 0;
    iter->key = NULL;
    iter->value = NULL;

    return CORE_SUCCESS;
}
