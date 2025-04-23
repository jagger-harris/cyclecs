#include "core/app/time.h"
#include "GLFW/glfw3.h"
#include "core/util/logger.h"

struct time {
    float delta;
    float total;
    double last_frame;
};

err time_new(time **out, arena *mem) {
    err err = CORE_SUCCESS;

    if (!out || !mem) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(time), _Alignof(time));
    if (err)
        goto err;

    (*out)->delta = 0.0F;
    (*out)->total = 0.0F;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to make new time", err);
    return err;
}

err time_get_dt(float *out, time *in) {
    err err = CORE_SUCCESS;

    if (!out || !in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    *out = in->delta;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to get dt from time", err);
    return err;
}

err time_update(time *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    double now = glfwGetTime();
    in->delta = (float)(now - in->last_frame);
    in->total += in->delta;
    in->last_frame = now;
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to update time", err);
    return err;
}
