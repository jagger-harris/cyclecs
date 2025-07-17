#include "core/util/table.h"
#include "core/util/logger.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_MIN_CAPACITY 16
#define TABLE_MIN_LOAD_FACTOR 0.25
#define TABLE_MAX_LOAD_FACTOR 0.75
#define TABLE_GROWTH_FACTOR 2
#define TABLE_SHRINK_FACTOR 0.5

static uint32_t table_hash(const void *key, size_t key_size) {
    const uint8_t *bytes = key;
    uint32_t hash = 2166136261U;

    for (size_t i = 0; i < key_size; ++i) {
        hash ^= (uint8_t)bytes[i];
        hash *= 16777619;
    }

    return hash;
}

static err table_get_key(void **out, const struct table *in, size_t index) {
    err status = CORE_SUCCESS;

    if (!out || !in) {
        status = CORE_NULLPTR;
        goto err;
    }

    *out = (uint8_t *)in->data + index * (in->key_size + in->value_size);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Getting table key failed");
    return status;
}

static err table_get_value(void **out, const struct table *in, size_t index) {
    err status = CORE_SUCCESS;

    if (!out || !in) {
        status = CORE_NULLPTR;
        goto err;
    }

    *out = (uint8_t *)in->data + index * (in->key_size + in->value_size) +
           in->key_size;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Getting table value failed");
    return status;
}

static err table_resize(struct table *in, size_t new_capacity) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct table new_table = {0};
    status = table_init(&new_table, new_capacity, in->key_size, in->value_size);
    if (status)
        goto err;

    for (size_t i = 0; i < in->capacity; ++i) {
        if (in->metadata[i] != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            status = table_get_key(&key_ptr, in, i);
            if (status)
                goto err;

            status = table_get_value(&value_ptr, in, i);
            if (status)
                goto err;

            status = table_insert(&new_table, key_ptr, value_ptr);
            if (status) {
                goto cleanup;
            }
        }
    }

    free(in->metadata);
    free(in->data);
    memcpy(in, &new_table, sizeof(struct table));
    return status;

cleanup:
    table_destroy(&new_table);

err:
    logger_log_err(LOGGER_ERR, status, "Resizing table failed");
    return status;
}

err table_init(struct table *out, size_t start_capacity, size_t key_size,
               size_t value_size) {
    err status = CORE_SUCCESS;

    out->capacity = start_capacity;
    out->length = 0;
    out->key_size = key_size;
    out->value_size = value_size;
    out->metadata = malloc(start_capacity * sizeof(uint8_t));
    if (!out->metadata) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }
    out->data = malloc(start_capacity * (key_size + value_size));
    if (!out->data) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    for (size_t i = 0; i < out->capacity; ++i)
        memset(&out->metadata[i], 0, sizeof(uint8_t));

    return status;

err:

    if (out->metadata)
        free(out->metadata);

    if (out->data)
        free(out->data);

    logger_log_err(LOGGER_ERR, status, "Init table failed");
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

err table_insert(struct table *in, const void *key, const void *value) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if ((float)in->length / in->capacity > TABLE_MAX_LOAD_FACTOR) {
        status = table_resize(in, in->capacity * TABLE_GROWTH_FACTOR);
        if (status)
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

            status = table_get_key(&key_ptr, in, probe);
            if (status)
                goto err;

            status = table_get_value(&value_ptr, in, probe);
            if (status)
                goto err;

            memcpy(key_ptr, key, in->key_size);
            memcpy(value_ptr, value, in->value_size);

            in->metadata[probe] = fingerprint;
            in->length++;

            return status;
        }
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Inserting table element failed");
    return status;
}

err table_remove(void *out, struct table *in, const void *key) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t hash_metadata = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            return status;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        status = table_get_key(&key_ptr, in, probe);
        if (status)
            goto err;

        status = table_get_value(&value_ptr, in, probe);
        if (status)
            goto err;

        if (probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            in->length--;
            in->metadata[probe] = 0;

            if ((float)in->length / in->capacity < TABLE_MIN_LOAD_FACTOR) {
                status = table_resize(in, in->capacity * TABLE_SHRINK_FACTOR);
                if (status)
                    goto err;
            }

            if (out)
                memcpy(out, value_ptr, in->value_size);

            return status;
        }
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Removing table element failed");
    return status;
}

err table_find(void *out, const struct table *in, const void *key) {
    err status = CORE_SUCCESS;

    if (!out || !in || !key) {
        status = CORE_NULLPTR;
        goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t fingerprint = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            return status;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        status = table_get_key(&key_ptr, in, probe);
        if (status)
            goto err;

        status = table_get_value(&value_ptr, in, probe);
        if (status)
            goto err;

        if (probe_metadata == fingerprint &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            memcpy(out, value_ptr, in->value_size);
            return status;
        }
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Finding table element failed");
    return status;
}

err table_find_ptr(void **out, const struct table *in, const void *key) {
    err status = CORE_SUCCESS;

    if (!out || !in || !key) {
        status = CORE_NULLPTR;
        goto err;
    }

    uint32_t hash = table_hash(key, in->key_size);
    uint8_t fingerprint = (hash & 0x7F) | 0x80;
    uint32_t index = hash & (in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        uint32_t probe = (index + i) & (in->capacity - 1);
        uint8_t probe_metadata = in->metadata[probe];

        if (probe_metadata == 0) {
            return status;
        }

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        status = table_get_key(&key_ptr, in, probe);
        if (status)
            goto err;

        status = table_get_value(&value_ptr, in, probe);
        if (status)
            goto err;

        if (probe_metadata == fingerprint &&
            memcmp(key_ptr, key, in->key_size) == 0) {
            *out = value_ptr;
            return status;
        }
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Finding table element ptr failed");
    return status;
}
