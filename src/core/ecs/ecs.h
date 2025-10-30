#ifndef ECS_H
#define ECS_H

#include "core/util/globals.h"
#include "core/util/table.h"
#include "core/util/types.h"

struct app;
struct ecs_world;

struct ecs {
    struct table worlds;
};

int ecs_init(struct ecs *out);
void ecs_destroy(struct ecs *in);
int ecs_add_world(struct ecs *in, u32 world_id, float tick_rate, int priority,
                  bool should_update);
int ecs_get_world(struct ecs_world **out, struct ecs *in, u32 world_id);
int ecs_remove_world(struct ecs *in, u32 world_id);
int ecs_get_all_worlds(struct table **out, struct ecs *in);
int ecs_update_all_worlds(struct ecs *in, struct app *app);

#endif // ECS_H
