#include "core/app/time.h"
#include "core/util/logger.h"
#include <GLFW/glfw3.h>

err time_new(struct time *out) {
    err status = CORE_SUCCESS;

    if (!out) {
        status = CORE_NULLPTR;
        goto err;
    }

    out->delta = 0.0F;
    out->total = 0.0F;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init time failed");
    return status;
}

err time_update(struct time *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    double now = glfwGetTime();
    in->delta = (float)(now - in->last_frame);
    in->total += in->delta;
    in->last_frame = now;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Updating time failed");
    return status;
}
