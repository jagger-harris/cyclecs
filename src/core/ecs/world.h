#ifndef ECS_WORLD_H
#define ECS_WORLD_H

#include "core/util/array.h"
#include "core/util/table.h"
#include "core/util/types.h"

struct app;

enum ecs_world_default_comp_types {
    ECS_COMP_NODE = 0,
    ECS_COMP_TRANSFORM,
    ECS_COMP_RENDERABLE,
    ECS_COMP_CAMERA,
    ECS_COMP_UI_BASE,
    ECS_COMP_UI_BUTTON,
    ECS_COMP_UI_LABEL,
    ECS_COMP_LENGTH
};
enum ecs_default_sys { ECS_SYS_UI_BUTTON = 0, ECS_SYS_LENGTH };

struct ecs_world {
    struct array entities;
    struct array free_entities;
    struct array pending_deletions;
    struct table components;
    struct table systems;
    u32 next_entity_id;
    int priority;
    float tick_rate;
    float tick_amount;
    bool should_update;
};

struct ecs_world_sparse_set {
    struct array sparse;
    struct array dense;
    struct array data;
    size_t component_size;
};

struct ecs_world_query {
    struct array sets;
    struct array component_ids;
    struct ecs_world_sparse_set *min_set;
    struct ecs_world *world;
    size_t count;
    size_t current_index;
};

typedef int (*ecs_world_system_callback)(struct ecs_world_query *query,
                                         struct app *app);

int ecs_world_init(struct ecs_world *out, float tick_rate, int priority,
                   bool should_update);
void ecs_world_destroy(struct ecs_world *in);
int ecs_world_entity_add(u32 *out, struct ecs_world *in);
int ecs_world_entity_remove(struct ecs_world *in, u32 entity);
int ecs_world_component_type_add(struct ecs_world *in, u32 component_id,
                                 size_t component_size);
int ecs_world_component_type_remove(struct ecs_world *in, u32 component_id);
int ecs_world_component_add(struct ecs_world *in, u32 entity, u32 component_id,
                            void *data);
int ecs_world_component_remove(struct ecs_world *in, u32 entity,
                               u32 component_id);
int ecs_world_query_init(struct ecs_world_query *out, struct ecs_world *world,
                         size_t count, ...);
void ecs_world_query_destroy(struct ecs_world_query *query);
int ecs_world_query_next(u32 *out, struct ecs_world_query *query);
int ecs_world_query_get(void **out, const struct ecs_world_query *query,
                        u32 component_id, u32 entity);
int ecs_world_query_data(void **out, const struct ecs_world *in, u32 entity,
                         u32 component_id);
int ecs_world_query_all_data(struct array **out, const struct ecs_world *in,
                             u32 component_id);
int ecs_world_query_all_entities(struct array **out, const struct ecs_world *in,
                                 u32 component_id);
int ecs_world_system_add(struct ecs_world *in, u32 system_id,
                         ecs_world_system_callback callback, size_t query_count,
                         ...);
int ecs_world_system_remove(struct ecs_world *in, u32 system_id);
int ecs_world_update(struct ecs_world *in, struct app *app);

#endif // ECS_WORLD_H
