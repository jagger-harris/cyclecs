#ifndef ECS_H
#define ECS_H

#include "core/util/array.h"
#include "core/util/err.h"
#include <stdint.h>

struct ecs {
    void *handles;
    struct array entities;
    struct array removed;
    struct array comps;
    struct array systems;
    uint32_t next_entity_id;
};

typedef int (*ecs_system_fn)(const struct ecs *, void *);

err ecs_init(struct ecs *out);
void ecs_destroy(struct ecs *in);
err ecs_init_handles(struct ecs *in, void *handles, size_t handles_size);
err ecs_entity_add(uint32_t *out, struct ecs *in);
err ecs_entity_remove(struct ecs *in, uint32_t entity);
err ecs_comp_type_add(uint32_t *out, struct ecs *in, size_t comp_size);
err ecs_comp_type_remove(struct ecs *in, uint32_t type);
err ecs_comp_add(struct ecs *in, uint32_t entity, uint32_t type, void *data);
err ecs_comp_remove(struct ecs *in, uint32_t entity, uint32_t type);
err ecs_comps_get(struct array **out, const struct ecs *in, uint32_t type);
err ecs_system_add(uint32_t *out, struct ecs *in, ecs_system_fn fn, void *ctx);
err ecs_system_remove(struct ecs *in, uint32_t type);
err ecs_update_all(const struct ecs *in);

#endif // ECS_H
