#ifndef ECS_SYSTEM_MOVE_H
#define ECS_SYSTEM_MOVE_H

#include "core/ecs/component/transform2d.h"
#include "core/ecs/world.h"
#include "core/util/error.h"
#include <GLFW/glfw3.h>
#include <math.h>

int move_sys(const struct ecs_world *in) {
    if (!in)
        return CORE_NULLPTR;

    // struct array *transform2ds = NULL;
    // int status =
    //     ECS_WORLD_QUERY_ALL_DATA(&transform2ds, in, struct transform2d);
    // if (status)
    //     return status;

    // struct transform2d *transforms_data = transform2ds->data;
    // for (size_t i = 0; i < transform2ds->length; ++i) {
    // transforms_data[i].pos[0] = sin(glfwGetTime());
    // }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_MOVE_H
