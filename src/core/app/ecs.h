#ifndef APP_ECS_H
#define APP_ECS_H

#include "core/util/array.h"
#include "core/util/table.h"
#include "core/util/types.h"

struct app;

struct ecs {
    struct table worlds;
};

struct ecs_world {
    struct array entities;
    struct array free_entities;
    struct table components;
    struct table systems;
    u32 next_entity_id;
    bool should_update;
};

struct ecs_world_sparse_set {
    struct array sparse;
    struct array dense;
    struct array data;
    size_t component_size;
};

int ecs_init(struct ecs *out);
void ecs_destroy(struct ecs *in);
int ecs_add_world(struct ecs *in, const char *id);
int ecs_get_world(struct ecs_world **out, struct ecs *in, const char *id);
int ecs_remove_world(struct ecs *in, const char *id);
int ecs_get_all_worlds(struct table **out, struct ecs *in);
int ecs_update_all_worlds(struct ecs *in, struct app *app);

typedef int (*ecs_world_system_callback)(struct ecs_world *world,
                                         struct app *app);

int ecs_world_init(struct ecs_world *out);
void ecs_world_destroy(struct ecs_world *in);
int ecs_world_entity_add(u32 *out, struct ecs_world *in);
int ecs_world_entity_remove(struct ecs_world *in, u32 entity);
int ecs_world_component_type_add(struct ecs_world *in, const char *component_id,
                                 size_t component_size);
int ecs_world_component_type_remove(struct ecs_world *in,
                                    const char *component_id);
int ecs_world_component_add(struct ecs_world *in, u32 entity,
                            const char *component_id, void *data);
int ecs_world_component_remove(struct ecs_world *in, u32 entity,
                               const char *component_id);
int ecs_world_query_data(void **out, const struct ecs_world *in, u32 entity,
                         const char *component_id);
int ecs_world_query_all_data(struct array **out, const struct ecs_world *in,
                             const char *component_id);
int ecs_world_query_all_entities(struct array **out, const struct ecs_world *in,
                                 const char *component_id);
int ecs_world_system_add(struct ecs_world *in, const char *system_id,
                         ecs_world_system_callback callback);
int ecs_world_system_remove(struct ecs_world *in, const char *system_id);
int ecs_world_update(struct ecs_world *in, struct app *app);

#endif // APP_ECS_H
