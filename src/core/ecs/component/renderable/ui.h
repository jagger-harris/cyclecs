#ifndef ECS_COMPONENT_UI_H
#define ECS_COMPONENT_UI_H

#include "core/util/array.h"
#include "core/util/globals.h"
#include "core/util/types.h"
#include <cglm/types.h>
#include <stdbool.h>

#define ECS_COMPONENT_UI_TEXT_MAX 128

struct ui_base {
    char id[GLOBALS_STR_ID_MAX];
    bool dirty;
    bool interactable;
};

struct ui_label {
    char text[ECS_COMPONENT_UI_TEXT_MAX];
    int font_size;
    u32 font_id;
    bool font;
};

struct ui_button {
    bool hovering;
    bool pressed;
    bool released;
    bool down;
};

struct ui_progress_bar {
    float value;
    float max_value;
};

#endif // ECS_COMPONENT_UI_H
