#ifndef CLS_ECS_H
#define CLS_ECS_H

#include <cls/util/table.h>
#include <cls/util/types.h>

static const u32 CLS_ENTITY_MAX = U32_MAX;
typedef u32 cls_entity;

struct cls_array;
struct cls_app;
struct cls_ecs;
struct cls_ecs_world;
struct cls_ecs_world_query;
struct cls_mem;

typedef int (*cls_ecs_world_iter_fn)(struct cls_ecs_world *world,
                                     void *user_data);
typedef int (*cls_ecs_world_system_fn)(struct cls_ecs_world_query *query,
                                       struct cls_app *app);

int cls_ecs_create(struct cls_ecs **ecs, struct cls_mem *mem);
void cls_ecs_destroy(struct cls_ecs *ecs);

int cls_ecs_world_add(struct cls_ecs_world **world, struct cls_ecs *ecs,
                      const char *id, bool should_update);
int cls_ecs_world_remove(struct cls_ecs *ecs, const char *id);
int cls_ecs_world_get(struct cls_ecs_world **world, const struct cls_ecs *ecs,
                      const char *id);
int cls_ecs_world_update_all(struct cls_ecs *ecs, struct cls_app *app);
int cls_ecs_world_iter_all(struct cls_ecs *ecs, cls_ecs_world_iter_fn fn,
                           void *user_data);
int cls_ecs_world_flush(struct cls_ecs_world *world);
int cls_ecs_world_entity_add(cls_entity *e, struct cls_ecs_world *world);
int cls_ecs_world_entity_remove(struct cls_ecs_world *world, cls_entity e);
int cls_ecs_world_component_type_add(struct cls_ecs_world *world,
                                     const char *id, size_t comp_size);
int cls_ecs_world_component_type_remove(struct cls_ecs_world *world,
                                        const char *id);
int cls_ecs_world_component_add(struct cls_ecs_world *world, cls_entity e,
                                const char *id, const void *comp_data);
int cls_ecs_world_component_remove(struct cls_ecs_world *world, cls_entity e,
                                   const char *id);
int cls_ecs_world_component_get(void **comp, const struct cls_ecs_world *world,
                                cls_entity e, const char *id);
int cls_ecs_world_query_create(struct cls_ecs_world_query **query,
                               struct cls_ecs_world *world, size_t comp_count,
                               const char *ids[]);
void cls_ecs_world_query_destroy(struct cls_ecs_world_query *query);
int cls_ecs_world_query_world_get(struct cls_ecs_world **world,
                                  struct cls_ecs_world_query *query);
int cls_ecs_world_query_clear(struct cls_ecs_world_query *query);
int cls_ecs_world_query_next(cls_entity *e, void **comps,
                             struct cls_ecs_world_query *query);
int cls_ecs_world_system_add(struct cls_ecs_world *world, const char *id,
                             cls_ecs_world_system_fn system, float tick_rate,
                             size_t comp_count, const char *ids[]);
int cls_ecs_world_system_remove(struct cls_ecs_world *world, const char *id);
int cls_ecs_world_singleton_add(struct cls_ecs_world *world, const char *id,
                                const void *data, size_t size);
int cls_ecs_world_singleton_remove(struct cls_ecs_world *world, const char *id);
int cls_ecs_world_singleton_get(void **comp, const struct cls_ecs_world *world,
                                const char *id);
int cls_ecs_world_update(struct cls_ecs_world *world, struct cls_app *app);
int cls_ecs_world_entities_length_get(size_t *len,
                                      const struct cls_ecs_world *world);
int cls_ecs_world_free_entities_length_get(size_t *len,
                                           const struct cls_ecs_world *world);

#endif // CLS_ECS_H
