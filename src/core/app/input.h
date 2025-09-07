#ifndef APP_INPUT_H
#define APP_INPUT_H

#include "core/util/error.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define KEYS GLFW_KEY_LAST + 1
#define MOUSE_BUTTONS GLFW_MOUSE_BUTTON_LAST + 1

struct input {
    bool keys[KEYS];
    bool mouse_buttons[MOUSE_BUTTONS];
    vec2 cursor_pos;
};

int input_key(struct input *in, int key, int action);
int input_mouse_button(struct input *in, int button, int action);

#endif // APP_INPUT_H
