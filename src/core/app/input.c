#include "core/app/input.h"
#include "core/util/error.h"
#include <GLFW/glfw3.h>
#include <cglm/types.h>
#include <stdalign.h>
#include <string.h>

int input_init(struct input *out) {
    if (!out)
        return CORE_NULLPTR;

    *out = (struct input){0};
    return CORE_SUCCESS;
}

int input_update(struct input *in) {
    if (!in)
        return CORE_NULLPTR;

    memcpy(in->last_keys, in->keys, sizeof(in->keys));
    memcpy(in->last_mouse_buttons, in->mouse_buttons,
           sizeof(in->mouse_buttons));

    glfwPollEvents();
    return CORE_SUCCESS;
}

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

int input_scroll_offset(vec2 out, struct input *in) {
    if (!in)
        return CORE_NULLPTR;

    out[0] = in->scroll_offset[0];
    out[1] = in->scroll_offset[1];
    in->scroll_offset[0] = 0.0f;
    in->scroll_offset[1] = 0.0f;
    return CORE_SUCCESS;
}

int input_key_pressed(bool *out, const struct input *in, int key) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->keys[key] && !in->last_keys[key];
    return CORE_SUCCESS;
}

int input_key_released(bool *out, const struct input *in, int key) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = !in->keys[key] && in->last_keys[key];
    return CORE_SUCCESS;
}

int input_key_down(bool *out, const struct input *in, int key) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->keys[key] && in->last_keys[key];
    return CORE_SUCCESS;
}

int input_mouse_pressed(bool *out, const struct input *in, int button) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->mouse_buttons[button] && !in->last_mouse_buttons[button];
    return CORE_SUCCESS;
}

int input_mouse_released(bool *out, const struct input *in, int button) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = !in->mouse_buttons[button] && in->last_mouse_buttons[button];
    return CORE_SUCCESS;
}

int input_mouse_down(bool *out, const struct input *in, int button) {
    if (!out || !in)
        return CORE_NULLPTR;

    *out = in->mouse_buttons[button] && in->last_mouse_buttons[button];
    return CORE_SUCCESS;
}
