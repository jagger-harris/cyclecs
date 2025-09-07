#include "core/app/input.h"
#include "core/util/error.h"

int input_key(struct input *in, int key, int action) {
    if (!in)
        return CORE_NULLPTR;

    if (action == GLFW_PRESS)
        in->keys[key] = true;

    if (action == GLFW_RELEASE)
        in->keys[key] = false;

    return CORE_SUCCESS;
}

int input_mouse_button(struct input *in, int button, int action) {
    if (!in)
        return CORE_NULLPTR;

    if (action == GLFW_PRESS)
        in->mouse_buttons[button] = true;

    if (action == GLFW_RELEASE)
        in->mouse_buttons[button] = false;

    return CORE_SUCCESS;
}
