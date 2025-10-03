#ifndef APP_INPUT_H
#define APP_INPUT_H

#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define KEYS (GLFW_KEY_LAST + 1)
#define MOUSE_BUTTONS (GLFW_MOUSE_BUTTON_LAST + 1)

struct input {
    bool keys[KEYS];
    bool last_keys[KEYS];
    bool mouse_buttons[MOUSE_BUTTONS];
    bool last_mouse_buttons[MOUSE_BUTTONS];
    vec2 cursor_pos;
    vec2 scroll_offset;
};

int input_init(struct input *out);
int input_update(struct input *in);
int input_key(struct input *in, int key, int action);
int input_mouse_button(struct input *in, int button, int action);
int input_scroll_offset(vec2 out, struct input *in);
int input_key_pressed(bool *out, const struct input *in, int key);
int input_key_released(bool *out, const struct input *in, int key);
int input_key_down(bool *out, const struct input *in, int key);
int input_mouse_pressed(bool *out, const struct input *in, int button);
int input_mouse_released(bool *out, const struct input *in, int button);
int input_mouse_down(bool *out, const struct input *in, int button);

#endif // APP_INPUT_H
