#include "core/util/table.h"
#include "core/util/logger.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MIN_CAPACITY 16
#define TABLE_MIN_LOAD_FACTOR 0.25
#define TABLE_MAX_LOAD_FACTOR 0.75
#define TABLE_GROWTH_FACTOR 2
#define TABLE_SHRINK_FACTOR 0.5

struct table {
    uint8_t *data;
    uint8_t *metadata;
    size_t capacity;
    size_t length;
    size_t key_size;
    size_t value_size;
};

static uint32_t table_hash(const void *key, size_t key_size) {
    const uint8_t *bytes = key;
    uint32_t hash = 2166136261U;

    for (size_t i = 0; i < key_size; ++i) {
        hash ^= (uint8_t)bytes[i];
        hash *= 16777619;
    }

    return hash;
}

static err table_get_key(void **out, table *in, size_t index) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = (uint8_t *)in->data + index * (in->key_size + in->value_size);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get key from table", err);
    return err;
}

static err table_get_value(void **out, table *in, size_t index) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = (uint8_t *)in->data + index * (in->key_size + in->value_size) +
           in->key_size;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get key from table", err);
    return err;
}

static err table_resize(table *in, size_t new_capacity) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    table *new_table = NULL;
    err = table_new(&new_table, new_capacity, in->key_size, in->value_size);
    if (err)
        goto err;

    for (size_t i = 0; i < in->capacity; ++i) {
        if (in->metadata[i] != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            err = table_get_key(&key_ptr, in, i);
            if (err)
                goto err;

            err = table_get_value(&value_ptr, in, i);
            if (err)
                goto err;

            err = table_insert(new_table, key_ptr, value_ptr);
            if (err) {
                goto cleanup;
            }
        }
    }

    free(in->metadata);
    free(in->data);
    in = new_table;

    return err;

cleanup:
    table_delete(new_table);

err:
    logger_log(LOGGER_ERR, "Failed to resize table", err);
    return err;
}

err table_new(table **out, size_t start_capacity, size_t key_size,
              size_t value_size) {
    err err = CORE_SUCCESS;

    /* TODO: Use arena or other allocator if possible */
    *out = malloc(sizeof(table));
    if (!*out) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    (*out)->capacity = start_capacity;
    (*out)->length = 0;
    (*out)->key_size = key_size;
    (*out)->value_size = value_size;
    (*out)->metadata = malloc(start_capacity * sizeof(uint8_t));
    if (!(*out)->metadata) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }
    (*out)->data = malloc(start_capacity * (key_size + value_size));
    if (!(*out)->data) {
        err = CORE_OUT_OF_MEMORY;
        goto err;
    }

    for (size_t i = 0; i < (*out)->capacity; ++i)
        memset(&(*out)->metadata[i], 0, sizeof(uint8_t));

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new table", err);

    if ((*out)->metadata)
        free((*out)->metadata);

    if ((*out)->data)
        free((*out)->data);

    if (*out)
        free(out);

    return err;
}

err table_delete(table *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if (in->metadata)
        free(in->metadata);

    if (in->data)
        free(in->data);

    free(in);

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete table", err);
    return err;
}

err table_insert(table *in, const void *key, const void *value) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    if ((float)in->length / in->capacity > TABLE_MAX_LOAD_FACTOR) {
        err = table_resize(in, in->capacity * TABLE_GROWTH_FACTOR);
        if (err)
            goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t fingerprint = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            err = table_get_key(&key_ptr, in, probe);
            if (err)
                goto err;

            err = table_get_value(&value_ptr, in, probe);
            if (err)
                goto err;

            memcpy(key_ptr, key, in->key_size);
            memcpy(value_ptr, value, in->value_size);

            in->metadata[probe] = fingerprint;
            in->length++;

            return err;
        }
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to insert element into table", err);
    return err;
}

err table_remove(void *out, table *in, const void *key) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t hash_metadata = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            return err;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        err = table_get_key(&key_ptr, in, probe);
        if (err)
            goto err;

        err = table_get_value(&value_ptr, in, probe);
        if (err)
            goto err;

        if (probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            in->length--;
            in->metadata[probe] = 0;

            if ((float)in->length / in->capacity < TABLE_MIN_LOAD_FACTOR) {
                err = table_resize(in, in->capacity * TABLE_SHRINK_FACTOR);
                if (err)
                    goto err;
            }

            if (out)
                memcpy(out, value_ptr, in->value_size);

            return err;
        }
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to remove element from table", err);
    return err;
}

err table_find(void *out, table *in, const void *key) {
    err err = CORE_SUCCESS;

    if (!out || !in || !key) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t fingerprint = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            return err;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        err = table_get_key(&key_ptr, in, probe);
        if (err)
            goto err;

        err = table_get_value(&value_ptr, in, probe);
        if (err)
            goto err;

        if (probe_metadata == fingerprint &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            memcpy(out, value_ptr, in->value_size);
            return err;
        }
    }

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to find element in table", err);
    return err;
}
