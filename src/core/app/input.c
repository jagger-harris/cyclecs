#include "core/app/input.h"
#include "core/util/err.h"
#include "core/util/logger.h"

void input_key(struct input *in, int key, int action) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (action == GLFW_PRESS)
        in->keys[key] = true;

    if (action == GLFW_RELEASE)
        in->keys[key] = false;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Setting input key failed");
}

void input_mouse_button(struct input *in, int button, int action) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (action == GLFW_PRESS)
        in->mouse_buttons[button] = true;

    if (action == GLFW_RELEASE)
        in->mouse_buttons[button] = false;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Setting input mouse button failed");
}
