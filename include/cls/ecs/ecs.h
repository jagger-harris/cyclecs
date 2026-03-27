#ifndef CLS_ECS_H
#define CLS_ECS_H

#include <cls/util/table.h>
#include <cls/util/types.h>

#define CLS_ENTITY_MAX UINT32_MAX
typedef u32 cls_entity;

struct cls_allocator;
struct cls_array;
struct cls_app;
struct cls_ecs;
struct cls_ecs_scene;
struct cls_ecs_world;
struct cls_ecs_world_query;

typedef int (*cls_ecs_world_iter_fn)(struct cls_ecs_world *world, void *data);
typedef int (*cls_ecs_world_system_fn)(struct cls_ecs_world_query *query,
                                       struct cls_app *app, void *data);

int cls_ecs_create(struct cls_ecs **ecs, struct cls_allocator *alloc);
void cls_ecs_destroy(struct cls_ecs *ecs);

int cls_ecs_scene_create_from_world(struct cls_ecs_scene **scene,
                                    const struct cls_ecs_world *world,
                                    const char *id);
int cls_ecs_scene_create_from_query(struct cls_ecs_scene **out,
                                    struct cls_ecs_world_query *query,
                                    const char *id);
void cls_ecs_scene_destroy(struct cls_ecs_scene *scene);
int cls_ecs_scene_spawn(const struct cls_ecs_scene *scene,
                        struct cls_ecs_world *world);
int cls_ecs_scene_save(const struct cls_ecs_scene *scene, const char *path);
int cls_ecs_scene_load(struct cls_ecs_scene **scene, const char *id,
                       const char *path);

int cls_ecs_world_add(struct cls_ecs *ecs, const char *id, float tick_rate,
                      int priority, bool should_update);
int cls_ecs_world_remove(struct cls_ecs *ecs, const char *id);
int cls_ecs_world_get(struct cls_ecs_world **world, const struct cls_ecs *ecs,
                      const char *id);
int cls_ecs_world_get_all(struct cls_table **worlds, const struct cls_ecs *ecs);
int cls_ecs_world_update_all(struct cls_ecs *ecs, struct cls_app *app);
int cls_ecs_world_iter_all(struct cls_ecs *ecs, cls_ecs_world_iter_fn fn,
                           void *data);
int cls_ecs_world_entity_add(cls_entity *e, struct cls_ecs_world *world);
int cls_ecs_world_entity_remove(struct cls_ecs_world *world, cls_entity e);
int cls_ecs_world_component_type_add(struct cls_ecs_world *world,
                                     const char *id, size_t component_size);
int cls_ecs_world_component_type_remove(struct cls_ecs_world *world,
                                        const char *id);
int cls_ecs_world_component_add(struct cls_ecs_world *world, cls_entity e,
                                const char *id, const void *comp_data);
int cls_ecs_world_component_remove(struct cls_ecs_world *world, cls_entity e,
                                   const char *id);
int cls_ecs_world_component_get(void **comp, const struct cls_ecs_world *world,
                                cls_entity e, const char *id);
int cls_ecs_world_query_create(struct cls_ecs_world_query **query,
                               struct cls_ecs_world *world, size_t count, ...);
void cls_ecs_world_query_destroy(struct cls_ecs_world_query *query);
int cls_ecs_world_query_world_get(struct cls_ecs_world **world,
                                  struct cls_ecs_world_query *query);
int cls_ecs_world_query_next(cls_entity *e, struct cls_ecs_world_query *query);
int cls_ecs_world_query_component_get(void **comp,
                                      const struct cls_ecs_world_query *query,
                                      const char *id, cls_entity e);
int cls_ecs_world_system_add(struct cls_ecs_world *world, const char *id,
                             cls_ecs_world_system_fn system, void *data,
                             size_t query_count, ...);
int cls_ecs_world_system_remove(struct cls_ecs_world *world, const char *id);
int cls_ecs_world_update(struct cls_ecs_world *world, struct cls_app *app);
int cls_ecs_world_entities_length_get(size_t *len,
                                      const struct cls_ecs_world *world);

#endif // CLS_ECS_H
