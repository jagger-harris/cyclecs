#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <cls/util/xxhash32.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

static const double MIN_LOAD_FACTOR = 0.25;
static const double MAX_LOAD_FACTOR = 0.75;
static const double SHRINK_FACTOR = 0.5;
static const size_t GROWTH_FACTOR = 2;
static const size_t SLOT_METADATA_SIZE = 8;

struct cls_table {
    size_t capacity;
    size_t length;
    size_t key_size;
    size_t value_size;
    size_t slot_size;
    void *slots;
};

struct cls_table_iterator {
    const struct cls_table *t;
    size_t current_idx;
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

static int cls_table_get_slot(void **slot, const struct cls_table *t,
                              size_t idx) {
    if (!slot || !t)
        return CLS_NULLPTR;

    *slot = (u8 *)t->slots + idx * t->slot_size;
    return CLS_SUCCESS;
}

static int cls_table_get_metadata(u8 **meta, const struct cls_table *t,
                                  size_t idx) {
    if (!meta || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = cls_table_get_slot(&slot, t, idx);
    if (error)
        return error;

    *meta = (u8 *)slot;
    return CLS_SUCCESS;
}

static int cls_table_get_key(void **key, const struct cls_table *t,
                             size_t idx) {
    if (!key || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = cls_table_get_slot(&slot, t, idx);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    *key = (u8 *)slot + key_offset;
    return CLS_SUCCESS;
}

static int cls_table_get_value(void **value, const struct cls_table *t,
                               size_t idx) {
    if (!value || !t)
        return CLS_NULLPTR;

    void *slot = NULL;
    int error = cls_table_get_slot(&slot, t, idx);
    if (error)
        return error;

    const size_t align = alignof(max_align_t);
    size_t key_offset = align_round_up(SLOT_METADATA_SIZE, align);
    size_t value_offset = align_round_up(key_offset + t->key_size, align);
    *value = (u8 *)slot + value_offset;
    return CLS_SUCCESS;
}

static int cls_table_insert_no_resize(struct cls_table *t, const void *key,
                                      const void *value) {
    if (!t)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 idx = hash & ((u32)t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(idx + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = cls_table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = cls_table_get_key(&key_ptr, t, probe);
            if (error)
                return error;

            error = cls_table_get_value(&value_ptr, t, probe);
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

static int cls_table_resize(struct cls_table *t, size_t new_capacity) {
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

    struct cls_table new_table = {.capacity = new_capacity,
                                  .length = 0,
                                  .key_size = t->key_size,
                                  .value_size = t->value_size,
                                  .slot_size = t->slot_size,
                                  .slots = new_slots};

    for (size_t i = 0; i < t->capacity; ++i) {
        u8 *metadata = NULL;
        int error = cls_table_get_metadata(&metadata, t, i);
        if (error) {
            free(new_slots_mem);
            return error;
        }

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = cls_table_get_key(&key_ptr, t, i);
            if (error) {
                free(new_slots_mem);
                return error;
            }

            error = cls_table_get_value(&value_ptr, t, i);
            if (error) {
                free(new_slots_mem);
                return error;
            }

            error = cls_table_insert_no_resize(&new_table, key_ptr, value_ptr);
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

int cls_table_create(struct cls_table **t, size_t start_capacity,
                     size_t key_size, size_t value_size) {
    if (!t)
        return CLS_NULLPTR;

    if (start_capacity == 0 || key_size == 0 || value_size == 0)
        return CLS_INVALID_ARG;

    struct cls_table *table = malloc(sizeof(struct cls_table));
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
        int error = cls_table_get_metadata(&metadata, table, i);
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

void cls_table_destroy(struct cls_table *t) {
    if (!t)
        return;

    if (t->slots)
        free(t->slots);

    free(t);
}

int cls_table_insert(struct cls_table *t, const void *key, const void *value) {
    if (!t || !key || !value)
        return CLS_NULLPTR;

    if ((double)t->length / (double)t->capacity > MAX_LOAD_FACTOR) {
        int error = cls_table_resize(t, t->capacity * GROWTH_FACTOR);
        if (error)
            return error;
    }

    return cls_table_insert_no_resize(t, key, value);
}

int cls_table_remove(void *value, struct cls_table *t, const void *key) {
    if (!t || !key)
        return CLS_NULLPTR;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 hash_metadata = (hash & 0x7f) | 0x80;
    u32 idx = hash & (u32)(t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(idx + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = cls_table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = cls_table_get_key(&key_ptr, t, probe);
        if (error)
            return error;

        error = cls_table_get_value(&value_ptr, t, probe);
        if (error)
            return error;

        if (*probe_metadata == hash_metadata &&
            memcmp(key_ptr, key, t->key_size) == 0) {
            t->length--;
            *probe_metadata = 0;

            if (value)
                memcpy(value, value_ptr, t->value_size);

            if ((double)t->length / (double)t->capacity < MIN_LOAD_FACTOR) {
                error = cls_table_resize(
                    t, (size_t)((double)t->capacity * SHRINK_FACTOR));
                if (error)
                    return error;
            }

            return CLS_SUCCESS;
        }
    }

    return CLS_SUCCESS;
}

int cls_table_find(void **value, const struct cls_table *t, const void *key) {
    if (!value || !t || !key)
        return CLS_NULLPTR;

    if (t->key_size == 0)
        return CLS_INVALID_ARG;

    u32 hash = 0;
    int error = cls_xxhash32(&hash, key, t->key_size, 0);
    if (error)
        return error;

    u8 fingerprint = (hash & 0x7f) | 0x80;
    u32 idx = hash & (u32)(t->capacity - 1);

    for (size_t i = 0; i < t->capacity; ++i) {
        u32 probe = (u32)(idx + i) & (u32)(t->capacity - 1);
        u8 *probe_metadata = NULL;
        error = cls_table_get_metadata(&probe_metadata, t, probe);
        if (error)
            return error;

        if (*probe_metadata == 0)
            return CLS_SUCCESS;

        void *key_ptr = NULL;
        void *value_ptr = NULL;

        error = cls_table_get_key(&key_ptr, t, probe);
        if (error)
            return error;

        error = cls_table_get_value(&value_ptr, t, probe);
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

int cls_table_find_cpy(void *value, const struct cls_table *t,
                       const void *key) {
    void *value_ptr = NULL;
    int error = cls_table_find(&value_ptr, t, key);
    if (error)
        return error;

    if (value_ptr != NULL)
        memcpy(value, value_ptr, t->value_size);

    return CLS_SUCCESS;
}

int cls_table_clear(struct cls_table *t) {
    if (!t)
        return CLS_NULLPTR;

    for (size_t i = 0; i < t->capacity; ++i) {
        u8 *metadata = NULL;
        int error = cls_table_get_metadata(&metadata, t, i);
        if (error)
            return error;

        *metadata = 0;
    }

    t->length = 0;
    return CLS_SUCCESS;
}

int cls_table_iterator_create(struct cls_table_iterator **it,
                              const struct cls_table *t) {
    if (!it || !t)
        return CLS_NULLPTR;

    struct cls_table_iterator *iter = malloc(sizeof(struct cls_table_iterator));
    if (!iter)
        return CLS_OUT_OF_MEMORY;

    iter->t = t;
    iter->current_idx = 0;
    iter->key = NULL;
    iter->value = NULL;

    *it = iter;
    return CLS_SUCCESS;
}

void cls_table_iterator_destroy(struct cls_table_iterator *it) {
    if (!it)
        return;

    free(it);
}

int cls_table_iterator_next(bool *exists, struct cls_table_iterator *it) {
    if (!it || !it->t)
        return CLS_NULLPTR;

    while (it->current_idx < it->t->capacity) {
        u8 *metadata = NULL;
        int error = cls_table_get_metadata(&metadata, it->t, it->current_idx);
        if (error)
            return error;

        if (*metadata != 0) {
            void *key_ptr = NULL;
            void *value_ptr = NULL;

            error = cls_table_get_key(&key_ptr, it->t, it->current_idx);
            if (error)
                return error;

            error = cls_table_get_value(&value_ptr, it->t, it->current_idx);
            if (error)
                return error;

            it->key = key_ptr;
            it->value = value_ptr;
            it->current_idx++;
            *exists = true;
            return CLS_SUCCESS;
        }

        it->current_idx++;
    }

    it->key = NULL;
    it->value = NULL;
    *exists = false;
    return CLS_SUCCESS;
}

int cls_table_iterator_clear(struct cls_table_iterator *it) {
    if (!it)
        return CLS_NULLPTR;

    it->current_idx = 0;
    it->key = NULL;
    it->value = NULL;
    return CLS_SUCCESS;
}

int cls_table_iterator_key_get(void **key,
                               const struct cls_table_iterator *it) {
    if (!key || !it)
        return CLS_NULLPTR;

    *key = it->key;
    return CLS_SUCCESS;
}

int cls_table_iterator_value_get(void **value,
                                 const struct cls_table_iterator *it) {
    if (!value || !it)
        return CLS_NULLPTR;

    *value = it->value;
    return CLS_SUCCESS;
}
