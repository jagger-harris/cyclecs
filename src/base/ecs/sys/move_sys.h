#ifndef ECS_SYS_MOVE_H
#define ECS_SYS_MOVE_H

#include "base/game/constants.h"
#include "core/ecs/comp/transform_comp.h"
#include "core/ecs/ecs.h"
#include "core/util/err.h"
#include "core/util/logger.h"

err move_sys(const struct ecs *in, void *ctx) {
    err status = CORE_SUCCESS;
    struct ecs_handles *handles = (struct ecs_handles *)in->handles;

    if (ctx) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct array *tfs = NULL;
    status = ecs_comps_get(&tfs, in, handles->transformable);
    if (status)
        goto err;

    for (size_t i = 0; i < tfs->length; ++i) {
        struct transform_comp *tf = NULL;
        array_get_ptr((void **)&tf, tfs, i);
        tf->rot_deg += 0.01F;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Running move sys failed");
    return status;
}

#endif // ECS_SYS_MOVE_H
