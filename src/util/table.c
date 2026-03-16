#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <cls/util/xxhash32.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#define MIN_CAPACITY 16
#define MIN_LOAD_FACTOR 0.25
#define MAX_LOAD_FACTOR 0.75
#define GROWTH_FACTOR 2
#define SHRINK_FACTOR 0.5
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
    const struct table *t;
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

static int table_get_slot(void **slot, const struct table *t, size_t index) {
    if (!slot || !t)
        return CLS_NULLPTR;

    *slot = (u8 *)t->slots + index * t->slot_size;
    return CLS_SUCCESS;
}

static int table_get_metadata(u8 **meta, const struct table *t, size_t index) {
    if (!meta || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, t, index);
    if (error)
        return error;

    *meta = (u8 *)slot;
    return CLS_SUCCESS;
}

static int table_get_key(void **key, const struct table *t, size_t index) {
    if (!key || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, t, index);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    *key = (u8 *)slot + key_offset;
    return CLS_SUCCESS;
}

static int table_get_value(void **value, const struct table *t, size_t index) {
    if (!value || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = table_get_slot(&slot, t, index);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    size_t value_offset = align_round_up(key_offset + t->key_size, align);
    *value = (u8 *)slot + value_offset;
    return CLS_SUCCESS;
}

static int table_insert_no_resize(struct table *t, const void *key,
                                  const void *value) {
    if (!t)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & ((u32)t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, t, probe);
            if (error)
                return error;

            error = table_get_value(&value_ptr, t, probe);
            if (error)
                return error;

            memcpy(key_ptr, key, t->key_size);
            memcpy(value_ptr, value, t->value_size);

            *probe_metadata = fingerprint;
            t->length++;
            return CLS_SUCCESS;
        }
    }

    return CLS_OUT_OF_MEMORY;
}

static int table_resize(struct table *t, size_t new_capacity) {
    if (!t)
        return CLS_NULLPTR;

    const size_t align = alignof(max_align_t);
    size_t slots_size = new_capacity * t->slot_size + (align - 1);

    void *new_slots_mem = malloc(slots_size);
    if (!new_slots_mem)
        return CLS_OUT_OF_MEMORY;

    uintptr_t base = (uintptr_t)new_slots_mem;
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    void *new_slots = (void *)aligned;

    for (size_t i = 0; i < new_capacity; ++i) {
        u8 *metadata = (u8 *)new_slots + i * t->slot_size;
        *metadata = 0;
    }

    struct table new_table = {.capacity = new_capacity,
                              .length = 0,
                              .key_size = t->key_size,
                              .value_size = t->value_size,
                              .slot_size = t->slot_size,
                              .slots = new_slots};

    for (size_t i = 0; i < t->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, t, i);
        if (error) {
            free(new_slots_mem);
            return error;
        }

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, t, i);
            if (error) {
                free(new_slots_mem);
                return error;
            }

            error = table_get_value(&value_ptr, t, i);
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

    free(t->slots);
    t->slots = new_slots;
    t->capacity = new_capacity;
    return CLS_SUCCESS;
}

int table_create(struct table **t, size_t start_capacity, size_t key_size,
                 size_t value_size) {
    if (!t)
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

    *t = table;
    return CLS_SUCCESS;
}

void table_destroy(struct table *t) {
    if (!t)
        return;

    if (t->slots)
        free(t->slots);

    free(t);
}

int table_insert(struct table *t, const void *key, const void *value) {
    if (!t || !key || !value)
        return CLS_NULLPTR;

    if ((double)t->length / (double)t->capacity > MAX_LOAD_FACTOR) {
        int error = table_resize(t, t->capacity * GROWTH_FACTOR);
        if (error)
            return error;
    }

    return table_insert_no_resize(t, key, value);
}

int table_remove(void *value, struct table *t, const void *key) {
    if (!t || !key)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 hash_metadata = (hash & 0x7f) | 0x80;
    u32 index = hash & (u32)(t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = table_get_key(&key_ptr, t, probe);
        if (error)
            return error;

        error = table_get_value(&value_ptr, t, probe);
        if (error)
            return error;

        if (*probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, t->key_size) == 0) {
            t->length--;
            *probe_metadata = 0;

            if (value)
                memcpy(value, value_ptr, t->value_size);

            if ((double)t->length / (double)t->capacity < MIN_LOAD_FACTOR) {
                error = table_resize(
                    t, (size_t)((double)t->capacity * SHRINK_FACTOR));
                if (error)
                    return error;
            }

            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int table_find(void **value, const struct table *t, const void *key) {
    if (!value || !t || !key)
        return CLS_NULLPTR;

    if (t->key_size == 0)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    int error = xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 index = hash & (u32)(t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(index + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = table_get_key(&key_ptr, t, probe);
        if (error)
            return error;

        error = table_get_value(&value_ptr, t, probe);
        if (error)
            return error;

        if (*probe_metadata == fingerprint &&
            memcmp(key_ptr, key, t->key_size) == 0) {
            *value = value_ptr;
            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int table_find_cpy(void *value, const struct table *t, const void *key) {
    void *value_ptr = NULL;
    int error = table_find(&value_ptr, t, key);
    if (error)
        return error;

    if (value_ptr != NULL)
        memcpy(value, value_ptr, t->value_size);

    return CLS_SUCCESS;
}

int table_clear(struct table *t) {
    if (!t)
        return CLS_NULLPTR;

    for (size_t i = 0; i < t->capacity; ++i) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, t, i);
        if (error)
            return error;

        *metadata = 0;
    }

    t->length = 0;
    return CLS_SUCCESS;
}

int table_iterator_create(struct table_iterator **it, const struct table *t) {
    if (!it || !t)
        return CLS_NULLPTR;

    struct table_iterator *iter = malloc(sizeof(struct table_iterator));
    if (!iter)
        return CLS_OUT_OF_MEMORY;

    iter->t = t;
    iter->current_index = 0;
    iter->key = NULL;
    iter->value = NULL;

    *it = iter;
    return CLS_SUCCESS;
}

void table_iterator_destroy(struct table_iterator *it) {
    if (!it)
        return;

    free(it);
}

int table_iterator_next(bool *exists, struct table_iterator *it) {
    if (!it || !it->t)
        return CLS_NULLPTR;

    while (it->current_index < it->t->capacity) {
        u8 *metadata = NULL;
        int error = table_get_metadata(&metadata, it->t, it->current_index);
        if (error)
            return error;

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = table_get_key(&key_ptr, it->t, it->current_index);
            if (error)
                return error;

            error = table_get_value(&value_ptr, it->t, it->current_index);
            if (error)
                return error;

            it->key = key_ptr;
            it->value = value_ptr;
            it->current_index++;
            *exists = true;
            return CLS_SUCCESS;
        }

        it->current_index++;
    }

    it->key = NULL;
    it->value = NULL;
    *exists = false;
    return CLS_SUCCESS;
}

int table_iterator_clear(struct table_iterator *it) {
    if (!it)
        return CLS_NULLPTR;

    it->current_index = 0;
    it->key = NULL;
    it->value = NULL;
    return CLS_SUCCESS;
}

int table_iterator_key_get(void **key, const struct table_iterator *it) {
    if (!key || !it)
        return CLS_NULLPTR;

    *key = it->key;
    return CLS_SUCCESS;
}

int table_iterator_value_get(void **value, const struct table_iterator *it) {
    if (!value || !it)
        return CLS_NULLPTR;

    *value = it->value;
    return CLS_SUCCESS;
}
