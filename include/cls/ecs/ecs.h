#ifndef ECS_H
#define ECS_H

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

enum ecs_world_default_comp_types {
    CLS_ECS_COMP_GROUP = 0,
    CLS_ECS_COMP_TRANSFORM,
    CLS_ECS_COMP_RENDERABLE,
    CLS_ECS_COMP_CAMERA,
    CLS_ECS_COMP_CAMERA_ACTIVE,
    CLS_ECS_COMP_UI_BASE,
    CLS_ECS_COMP_UI_BUTTON,
    CLS_ECS_COMP_UI_BUTTON_GROUP,
    CLS_ECS_COMP_UI_LABEL,
    CLS_ECS_COMP_UI_LABEL_GROUP,
    CLS_ECS_COMP_LENGTH
};
enum ecs_default_sys { CLS_ECS_SYS_UI_BUTTON = 0, CLS_ECS_SYS_LENGTH };

typedef int (*ecs_world_iter_callback)(struct ecs_world *world, void *data);
typedef int (*ecs_world_system_fn)(struct ecs_world_query *query,
                                   struct app *app, void *data);

int ecs_create(struct ecs **out, struct allocator *alloc);
void ecs_destroy(struct ecs *in);

int ecs_scene_create_from_world(struct ecs_scene **out,
                                const struct ecs_world *in,
                                const char *scene_id);
int ecs_scene_create_from_query(struct ecs_scene **out,
                                struct ecs_world_query *in,
                                const char *scene_id);
void ecs_scene_destroy(struct ecs_scene *in);
int ecs_scene_spawn(const struct ecs_scene *in, struct ecs_world *world);
int ecs_scene_save(const struct ecs_scene *in, const char *path);
int ecs_scene_load(struct ecs_scene **out, const char *scene_id,
                   const char *path);

int ecs_world_add(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update);
int ecs_world_remove(struct ecs *in, u32 world_id);
int ecs_world_get(struct ecs_world **out, const struct ecs *in, u32 world_id);
int ecs_world_get_all(struct table **out, const struct ecs *in);
int ecs_world_update_all(struct ecs *in, struct app *app);
int ecs_world_iter_all(struct ecs *in, ecs_world_iter_callback callback,
                       void *data);
int ecs_world_entity_add(u32 *out, struct ecs_world *in);
int ecs_world_entity_remove(struct ecs_world *in, entity entity);
int ecs_world_component_type_add(struct ecs_world *in, u32 component_id,
                                 size_t component_size);
int ecs_world_component_type_remove(struct ecs_world *in, u32 component_id);
int ecs_world_component_add(struct ecs_world *in, entity e, u32 component_id,
                            const void *comp_data);
int ecs_world_component_remove(struct ecs_world *in, entity e,
                               u32 component_id);
int ecs_world_component_get(void **out, const struct ecs_world *in, entity e,
                            u32 component_id);
int ecs_world_query_create(struct ecs_world_query **out,
                           struct ecs_world *world, size_t count, ...);
void ecs_world_query_destroy(struct ecs_world_query *query);
int ecs_world_query_world_get(struct ecs_world **world,
                              struct ecs_world_query *query);
int ecs_world_query_next(entity *out, struct ecs_world_query *query);
int ecs_world_query_component_get(void **out,
                                  const struct ecs_world_query *query,
                                  u32 component_id, entity e);
int ecs_world_system_add(struct ecs_world *in, u32 system_id,
                         ecs_world_system_fn system, void *data,
                         size_t query_count, ...);
int ecs_world_system_remove(struct ecs_world *in, u32 system_id);
int ecs_world_update(struct ecs_world *in, struct app *app);
int ecs_world_entities_length_get(size_t *out, const struct ecs_world *in);

#endif // ECS_H
