#ifndef ECS_WORLD_H
#define ECS_WORLD_H

#include "core/util/array.h"
#include "core/util/table.h"
#include "core/util/types.h"
#include "core/util/xxhash64.h"
#include <stdbool.h>
#include <string.h>

struct ecs_world {
    struct table components;
    struct table systems;
    struct array entities;
    struct array free_entities;
    u64 next_entity_id;
    bool should_update;
};

struct ecs_world_sparse_set {
    struct array sparse;
    struct array dense;
    struct array data;
};

typedef int (*ecs_world_system_callback)(const struct ecs_world *world);

int ecs_world_init(struct ecs_world *out);
void ecs_world_destroy(struct ecs_world *in);
int ecs_world_entity_add(u64 *out, struct ecs_world *in);
int ecs_world_entity_remove(struct ecs_world *in, u64 entity);
int ecs_world_component_type_add(struct ecs_world *in, const char *component_id,
                                 size_t component_size);
int ecs_world_component_type_remove(struct ecs_world *in,
                                    const char *component_id);
int ecs_world_component_add(struct ecs_world *in, u64 entity,
                            const char *component_id, void *data);
int ecs_world_component_remove(struct ecs_world *in, u64 entity,
                               const char *component_id);
int ecs_world_query_data(void **out, const struct ecs_world *in, u64 entity,
                         const char *component_id);
int ecs_world_query_all_data(struct array **out, const struct ecs_world *in,
                             const char *component_id);
int ecs_world_query_all_entities(struct array **out, const struct ecs_world *in,
                                 const char *component_id);
int ecs_world_system_add(struct ecs_world *in, const char *system_id,
                         ecs_world_system_callback callback);
int ecs_world_system_remove(struct ecs_world *in, const char *system_id);
int ecs_world_update(const struct ecs_world *in);

#endif /* ECS_WORLD_H */
