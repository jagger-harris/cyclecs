#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <cls/util/xxhash32.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_MIN_CAPACITY 16
#define TABLE_MIN_LOAD_FACTOR 0.25
#define TABLE_MAX_LOAD_FACTOR 0.75
#define TABLE_GROWTH_FACTOR 2
#define TABLE_SHRINK_FACTOR 0.5
#define SLOT_METADATA_SIZE 8

struct table {
    size_t capacity;
    size_t length;
    size_t key_size;
    size_t value_size;
    size_t slot_size;
    void *slots;
};

struct table_iterator {
    const struct table *table;
    size_t current_index;
    void *key;
    void *value;
};

static inline size_t align_round_up(size_t x, size_t align) {
    return (x + (align - 1)) & ~(align - 1);
}

static size_t table_calculate_slot_size(size_t key_size, size_t value_size) {
    const size_t meta = SLOT_METADATA_SIZE;
    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(meta, align);
    size_t value_offset = align_round_up(key_offset + key_size, align);
    return value_offset + value_size;
}

static int table_get_slot(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = (u8 *)in->slots + index * in->slot_size;
    return CLS_SUCCESS;
}

static int table_get_metadata(u8 **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    *out = (u8 *)slot;
    return CLS_SUCCESS;
}

static int table_get_key(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    *out = (u8 *)slot + key_offset;
    return CLS_SUCCESS;
}

static int table_get_value(void **out, const struct table *in, size_t index) {
    if (!out || !in)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, in, index);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    size_t value_offset = align_round_up(key_offset + in->key_size, align);
    *out = (u8 *)slot + value_offset;
    return CLS_SUCCESS;
}

static int table_insert_no_resize(struct table *in, const void *key,
                                  const void *value) {
    if (!in)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = xxhash32(&hash, key, in->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & ((u32)in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(in->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, in, probe);
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
            return CLS_SUCCESS;
        }
    }

    return CLS_OUT_OF_MEMORY;
}

static int table_resize(struct table *in, size_t new_capacity) {
    if (!in)
        return CLS_NULLPTR;

    const size_t align = alignof(max_align_t);
    size_t slots_size = new_capacity * in->slot_size + (align - 1);

    void *new_slots_mem = malloc(slots_size);
    if (!new_slots_mem)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)new_slots_mem;
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    void *new_slots = (void *)aligned;

    for (size_t i = 0; i < new_capacity; ++i) {
        u8 *metadata = (u8 *)new_slots + i * in->slot_size;
        *metadata = 0;
    }

    struct table new_table = {.capacity = new_capacity,
                              .length = 0,
                              .key_size = in->key_size,
                              .value_size = in->value_size,
                              .slot_size = in->slot_size,
                              .slots = new_slots};

    for (size_t i = 0; i < in->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, in, i);
        if (error) {
            free(new_slots_mem);
            return error;
        }

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, in, i);
            if (error) {
                free(new_slots_mem);
                return error;
            }

            error = table_get_value(&value_ptr, in, i);
            if (error) {
                free(new_slots_mem);
                return error;
            }

            error = table_insert_no_resize(&new_table, key_ptr, value_ptr);
            if (error) {
                free(new_slots_mem);
                return error;
            }
        }
    }

    free(in->slots);
    in->slots = new_slots;
    in->capacity = new_capacity;
    return CLS_SUCCESS;
}

int table_create(struct table **out, size_t start_capacity, size_t key_size,
                 size_t value_size) {
    if (!out)
        return CLS_NULLPTR;

    if (start_capacity == 0 || key_size == 0 || value_size == 0)
        return CLS_INVALID_ARG;

    struct table *table = malloc(sizeof(struct table));
    if (!table)
        return CLS_OUT_OF_MEMORY;

    size_t slot_size = table_calculate_slot_size(key_size, value_size);

    table->capacity = start_capacity;
    table->length = 0;
    table->key_size = key_size;
    table->value_size = value_size;
    table->slot_size = slot_size;

    const size_t align = alignof(max_align_t);
    size_t slots_size = start_capacity * slot_size + (align - 1);

    void *slots_mem = malloc(slots_size);
    if (!slots_mem) {
        free(table);
        return CLS_OUT_OF_MEMORY;
    }

    uintptr_t base = (uintptr_t)slots_mem;
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    table->slots = (void *)aligned;

    for (size_t i = 0; i < start_capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, table, i);
        if (error) {
            free(slots_mem);
            free(table);
            return error;
        }

        *metadata = 0;
    }

    *out = table;
    return CLS_SUCCESS;
}

void table_destroy(struct table *in) {
    if (!in)
        return;

    if (in->slots)
        free(in->slots);

    free(in);
}

int table_insert(struct table *in, const void *key, const void *value) {
    if (!in || !key || !value)
        return CLS_NULLPTR;

    if ((double)in->length / (double)in->capacity > TABLE_MAX_LOAD_FACTOR) {
        int error = table_resize(in, in->capacity * TABLE_GROWTH_FACTOR);
        if (error)
            return error;
    }

    return table_insert_no_resize(in, key, value);
}

int table_remove(void *out, struct table *in, const void *key) {
    if (!in || !key)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = xxhash32(&hash, key, in->key_size, 0);
    if (error)
        return error;

    u8 hash_metadata = (hash & 0x7f) | 0x80;
    u32 index = hash & (u32)(in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(in->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, in, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

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

            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int table_find(void **out, const struct table *in, const void *key) {
    if (!out || !in || !key)
        return CLS_NULLPTR;

    if (in->key_size == 0)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    int error = xxhash32(&hash, key, in->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & (u32)(in->capacity - 1);

    for (size_t i = 0; i < in->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(in->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, in, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

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
            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int table_find_cpy(void *out, const struct table *in, const void *key) {
    void *value = NULL;
    int error = table_find(&value, in, key);
    if (error)
        return error;

    if (value != NULL)
        memcpy(out, value, in->value_size);

    return CLS_SUCCESS;
}

int table_clear(struct table *in) {
    if (!in)
        return CLS_NULLPTR;

    for (size_t i = 0; i < in->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, in, i);
        if (error)
            return error;

        *metadata = 0;
    }

    in->length = 0;
    return CLS_SUCCESS;
}

int table_iterator_create(struct table_iterator **out,
                          const struct table *table) {
    if (!out || !table)
        return CLS_NULLPTR;

    struct table_iterator *iter = malloc(sizeof(struct table_iterator));
    if (!iter)
        return CLS_OUT_OF_MEMORY;

    iter->table = table;
    iter->current_index = 0;
    iter->key = NULL;
    iter->value = NULL;

    *out = iter;
    return CLS_SUCCESS;
}

void table_iterator_destroy(struct table_iterator *in) {
    if (!in)
        return;

    free(in);
}

int table_iterator_next(bool *out, struct table_iterator *in) {
    if (!in || !in->table)
        return CLS_NULLPTR;

    while (in->current_index < in->table->capacity) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, in->table, in->current_index);
        if (error)
            return error;

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, in->table, in->current_index);
            if (error)
                return error;

            error = table_get_value(&value_ptr, in->table, in->current_index);
            if (error)
                return error;

            in->key = key_ptr;
            in->value = value_ptr;
            in->current_index++;
            *out = true;
            return CLS_SUCCESS;
        }

        in->current_index++;
    }

    in->key = NULL;
    in->value = NULL;
    *out = false;
    return CLS_SUCCESS;
}

int table_iterator_clear(struct table_iterator *in) {
    if (!in)
        return CLS_NULLPTR;

    in->current_index = 0;
    in->key = NULL;
    in->value = NULL;
    return CLS_SUCCESS;
}

int table_iterator_key_get(void **out, const struct table_iterator *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->key;
    return CLS_SUCCESS;
}

int table_iterator_value_get(void **out, const struct table_iterator *in) {
    if (!out || !in)
        return CLS_NULLPTR;

    *out = in->value;
    return CLS_SUCCESS;
}
