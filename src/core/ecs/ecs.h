#ifndef ECS_H
#define ECS_H

typedef unsigned int ecs_entity;
typedef unsigned int ecs_component_type;
typedef void *ecs_component;
typedef void (*ecs_system)(int);
typedef struct ecs ecs;

int ecs_new(ecs *out);
int ecs_delete(ecs *in);
int ecs_add_entity(ecs_entity *out, ecs *in);
int ecs_remove_entity(ecs *in, ecs_entity entity);
int ecs_add_component_type(ecs *in, ecs_component_type type);
int ecs_remove_component_type(ecs *in, ecs_component_type type);
int ecs_add_component(ecs *in, ecs_entity entity);
int ecs_remove_component(ecs *in, ecs_entity entity);
int ecs_add_system(ecs *in, ecs_system system);
int ecs_remove_system(ecs *in, ecs_system system);
int ecs_update_systems(ecs *in);

#endif /* ECS_H */
