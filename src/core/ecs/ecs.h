#ifndef ECS_H
#define ECS_H

#include "core/ecs/ecs_ctx.h"
#include "core/util/arena.h"
#include "core/util/array.h"
#include "core/util/err.h"
#include <stdint.h>

typedef struct ecs ecs;
typedef uint32_t ecs_entity;
typedef uint32_t ecs_comp_type;
typedef uint32_t ecs_system_type;
typedef int (*ecs_system)(ecs *, ecs_ctx *);

err ecs_new(ecs **out, arena *mem);
err ecs_delete(ecs *in);
err ecs_entity_add(ecs_entity *out, ecs *in);
err ecs_entity_remove(ecs *in, ecs_entity entity);
err ecs_comp_type_add(ecs_comp_type *out, ecs *in, size_t comp_size);
err ecs_comp_type_remove(ecs *in, ecs_comp_type type);
err ecs_comp_add(ecs *in, ecs_entity entity, ecs_comp_type type, void *data);
err ecs_comp_remove(ecs *in, ecs_entity entity, ecs_comp_type type);
err ecs_comps_get(array **out, ecs *in, ecs_comp_type type);
err ecs_system_add(ecs_system_type *out, ecs *in, ecs_system system);
err ecs_system_remove(ecs *in, ecs_system_type type);
err ecs_update(ecs *in, ecs_ctx *ctx);

#endif /* ECS_H */
