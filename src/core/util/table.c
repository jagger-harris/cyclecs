#include "core/util/table.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include "core/util/xxhash64.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MIN_CAPACITY 16
#define TABLE_MIN_LOAD_FACTOR 0.25
#define TABLE_MAX_LOAD_FACTOR 0.75
#define TABLE_GROWTH_FACTOR 2
#define TABLE_SHRINK_FACTOR 0.5

static int table_get_key(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = (u8 *)in->data + index * (in->key_size + in->value_size);
    return CORE_SUCCESS;
}

static int table_get_value(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out =
        (u8 *)in->data + index * (in->key_size + in->value_size) + in->key_size;
    return CORE_SUCCESS;
}

static int table_resize(struct table *in, size_t new_capacity) {
    if (!in)
        return CORE_NULLPTR;

    struct table new_table = {0};
    int status = CORE_SUCCESS;
    status = table_init(&new_table, new_capacity, in->key_size, in->value_size);
    if (status)
        goto cleanup;

    for (size_t i = 0; i < in->capacity; ++i) {
        if (in->metadata[i] != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            status = table_get_key(&key_ptr, in, i);
            if (status)
                goto cleanup;

            status = table_get_value(&value_ptr, in, i);
            if (status)
                goto cleanup;

            status = table_insert(&new_table, key_ptr, value_ptr);
            if (status)
                goto cleanup;
        }
    }

    free(in->metadata);
    free(in->data);
    memcpy(in, &new_table, sizeof(struct table));
    return CORE_SUCCESS;

cleanup:
    table_destroy(&new_table);
    return status;
}

int table_init(struct table *out, size_t start_capacity, size_t key_size,
               size_t value_size) {
    if (!out)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;

    out->capacity = start_capacity;
    out->length = 0;
    out->key_size = key_size;
    out->value_size = value_size;
    out->metadata = malloc(start_capacity * sizeof(u8));
    if (!out->metadata) {
        status = CORE_OUT_OF_MEMORY;
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                         "Allocating table metadata failed");
        goto cleanup;
    }
    out->data = malloc(start_capacity * (key_size + value_size));
    if (!out->data) {
        status = CORE_OUT_OF_MEMORY;
        LOGGER_LOG_ERROR(LOGGER_ERROR, status, "%s",
                         "Allocating table data failed");
        goto cleanup;
    }

    for (size_t i = 0; i < out->capacity; ++i)
        memset(&out->metadata[i], 0, sizeof(u8));

    return CORE_SUCCESS;

cleanup:
    if (out->metadata)
        free(out->metadata);

    if (out->data)
        free(out->data);

    return status;
}

void table_destroy(struct table *in) {
    if (!in)
        return;

    if (in->metadata)
        free(in->metadata);

    if (in->data)
        free(in->data);
}

int table_insert(struct table *in, const void *key, const void *value) {
    if (!in)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    if ((float)in->length / in->capacity > TABLE_MAX_LOAD_FACTOR) {
        status = table_resize(in, in->capacity * TABLE_GROWTH_FACTOR);
        if (status)
            return status;
    }

    u64 hash = xxhash64(key, in->key_size, 0);
    u8 fingerprint = (hash & 0x7F) | 0x80;
    u64 index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u64 probe = (index + i) & (in->capacity - 1);
        u8 probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            status = table_get_key(&key_ptr, in, probe);
            if (status)
                return status;

            status = table_get_value(&value_ptr, in, probe);
            if (status)
                return status;

            memcpy(key_ptr, key, in->key_size);
            memcpy(value_ptr, value, in->value_size);

            in->metadata[probe] = fingerprint;
            in->length++;
            return CORE_SUCCESS;
        }
    }

    return CORE_SUCCESS;
}

int table_remove(void *out, struct table *in, const void *key) {
    if (!in || !key)
        return CORE_NULLPTR;

    u64 hash = xxhash64(key, in->key_size, 0);
    u8 hash_metadata = (hash & 0x7F) | 0x80;
    u64 index = hash & (in->capacity - 1);

    int status = CORE_SUCCESS;
    u64 probe = 0;
    u8 probe_metadata = 0;
    for (size_t i = 0; i < in->capacity; ++i) {
        probe = (index + i) & (in->capacity - 1);
        probe_metadata = in->metadata[probe];

        if (probe_metadata == 0)
            return CORE_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        status = table_get_key(&key_ptr, in, probe);
        if (status)
            return status;

        status = table_get_value(&value_ptr, in, probe);
        if (status)
            return status;

        if (probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            in->length--;
            in->metadata[probe] = 0;

            if ((float)in->length / in->capacity < TABLE_MIN_LOAD_FACTOR) {
                status = table_resize(in, in->capacity * TABLE_SHRINK_FACTOR);
                if (status)
                    return status;
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

    u64 hash = xxhash64(key, in->key_size, 0);
    u8 fingerprint = (hash & 0x7F) | 0x80;
    u64 index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u64 probe = (index + i) & (in->capacity - 1);
        u8 probe_metadata = in->metadata[probe];
        if (probe_metadata == 0)
            return CORE_NOT_FOUND;

        void *key_ptr = NULL;
        void *value_ptr = NULL;
        int status = CORE_SUCCESS;
        status = table_get_key(&key_ptr, in, probe);
        if (status)
            return status;

        status = table_get_value(&value_ptr, in, probe);
        if (status)
            return status;

        if (probe_metadata == fingerprint &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            *out = value_ptr;
            return status;
        }
    }

    return CORE_NOT_FOUND;
}

int table_find_cpy(void *out, const struct table *in, const void *key) {
    void *value = NULL;
    int status = CORE_SUCCESS;
    status = table_find(&value, in, key);
    if (status)
        return status;

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
        if (iter->table->metadata[iter->current_index] != 0) {
            int status = CORE_SUCCESS;

            status =
                table_get_key(&iter->key, iter->table, iter->current_index);
            if (status)
                return false;

            status =
                table_get_value(&iter->value, iter->table, iter->current_index);
            if (status)
                return false;

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

void *table_iterator_get_key(const struct table_iterator *iter) {
    return iter ? iter->key : NULL;
}

void *table_iterator_get_value(const struct table_iterator *iter) {
    return iter ? iter->value : NULL;
}

int table_iterator_get_key_cpy(const struct table_iterator *iter, void *out) {
    if (!iter || !out || !iter->key)
        return CORE_NULLPTR;

    memcpy(out, iter->key, iter->table->key_size);
    return CORE_SUCCESS;
}

int table_iterator_get_value_cpy(const struct table_iterator *iter, void *out) {
    if (!iter || !out || !iter->value)
        return CORE_NULLPTR;

    memcpy(out, iter->value, iter->table->value_size);
    return CORE_SUCCESS;
}
