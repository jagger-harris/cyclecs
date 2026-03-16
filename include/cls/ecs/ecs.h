#ifndef CLS_ECS_H
#define CLS_ECS_H

#include <cls/util/table.h>
#include <cls/util/types.h>

#define ENTITY_MAX UINT32_MAX
typedef u32 entity;

struct allocator;
struct array;
struct app;
struct ecs;
struct ecs_scene;
struct ecs_world;
struct ecs_world_query;

typedef int (*ecs_world_iter_callback)(struct ecs_world *world, void *data);
typedef int (*ecs_world_system_fn)(struct ecs_world_query *query,
                                   struct app *app, void *data);

int ecs_create(struct ecs **ecs, struct allocator *alloc);
void ecs_destroy(struct ecs *ecs);

int ecs_scene_create_from_world(struct ecs_scene **scene,
                                const struct ecs_world *world, const char *id);
int ecs_scene_create_from_query(struct ecs_scene **out,
                                struct ecs_world_query *query, const char *id);
void ecs_scene_destroy(struct ecs_scene *scene);
int ecs_scene_spawn(const struct ecs_scene *scene, struct ecs_world *world);
int ecs_scene_save(const struct ecs_scene *scene, const char *path);
int ecs_scene_load(struct ecs_scene **scene, const char *id, const char *path);

int ecs_world_add(struct ecs *ecs, const char *id, float tick_rate,
                  int priority, bool should_update);
int ecs_world_remove(struct ecs *ecs, const char *id);
int ecs_world_get(struct ecs_world **world, const struct ecs *ecs,
                  const char *id);
int ecs_world_get_all(struct table **worlds, const struct ecs *ecs);
int ecs_world_update_all(struct ecs *ecs, struct app *app);
int ecs_world_iter_all(struct ecs *ecs, ecs_world_iter_callback callback,
                       void *data);
int ecs_world_entity_add(entity *e, struct ecs_world *world);
int ecs_world_entity_remove(struct ecs_world *world, entity e);
int ecs_world_component_type_add(struct ecs_world *world, const char *id,
                                 size_t component_size);
int ecs_world_component_type_remove(struct ecs_world *world, const char *id);
int ecs_world_component_add(struct ecs_world *world, entity e, const char *id,
                            const void *comp_data);
int ecs_world_component_remove(struct ecs_world *world, entity e,
                               const char *id);
int ecs_world_component_get(void **comp, const struct ecs_world *world,
                            entity e, const char *id);
int ecs_world_query_create(struct ecs_world_query **query,
                           struct ecs_world *world, size_t count, ...);
void ecs_world_query_destroy(struct ecs_world_query *query);
int ecs_world_query_world_get(struct ecs_world **world,
                              struct ecs_world_query *query);
int ecs_world_query_next(entity *e, struct ecs_world_query *query);
int ecs_world_query_component_get(void **comp,
                                  const struct ecs_world_query *query,
                                  const char *id, entity e);
int ecs_world_system_add(struct ecs_world *world, const char *id,
                         ecs_world_system_fn system, void *data,
                         size_t query_count, ...);
int ecs_world_system_remove(struct ecs_world *world, const char *id);
int ecs_world_update(struct ecs_world *world, struct app *app);
int ecs_world_entities_length_get(size_t *len, const struct ecs_world *world);

#endif // CLS_ECS_H
