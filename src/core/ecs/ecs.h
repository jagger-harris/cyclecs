#ifndef ECS_H
#define ECS_H

#include "core/util/globals.h"
#include "core/util/table.h"
#include "core/util/types.h"

struct array;
struct app;
struct ecs;
struct ecs_world;
struct mem;

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

typedef int (*ecs_world_iter_callback)(struct ecs_world *world, void *data);

struct ecs_world_query {
    struct array *sets;
    struct array *component_ids;
    struct ecs_world_sparse_set *min_set;
    struct ecs_world *world;
    size_t count;
    size_t current_index;
};

int ecs_create(struct ecs **out, struct mem *mem);
void ecs_destroy(struct ecs *in);
int ecs_world_add(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update);
int ecs_world_remove(struct ecs *in, u32 world_id);
int ecs_world_get(struct ecs_world **out, struct ecs *in, u32 world_id);
int ecs_get_all_worlds(struct table **out, struct ecs *in);
int ecs_update_all_worlds(struct ecs *in, struct app *app);
int ecs_iter_all_worlds(struct ecs *in, ecs_world_iter_callback callback,
                        void *data);

typedef int (*ecs_world_system_fn)(struct ecs_world_query *query,
                                   struct app *app);

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
int ecs_world_query_get_single(void **out, const struct ecs_world *in,
                               u32 entity, u32 component_id);
int ecs_world_system_add(struct ecs_world *in, u32 system_id,
                         ecs_world_system_fn system, size_t query_count, ...);
int ecs_world_system_remove(struct ecs_world *in, u32 system_id);
int ecs_world_update(struct ecs_world *in, struct app *app);
int ecs_world_entities_length_get(size_t *out, struct ecs_world *in);

#endif // ECS_H
