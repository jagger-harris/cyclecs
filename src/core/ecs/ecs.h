#ifndef ECS_H
#define ECS_H

#include "core/ecs/world.h"
#include "core/util/table.h"
#include <stdint.h>

struct ecs {
    struct table worlds;
};

int ecs_init(struct ecs *out);
void ecs_destroy(struct ecs *in);
int ecs_add_world(struct ecs *in, const char *id);
int ecs_get_world(struct ecs_world **out, struct ecs *in, const char *id);
int ecs_remove_world(struct ecs *in, const char *id);
int ecs_update_all_worlds(struct ecs *in);

#endif // ECS_H
