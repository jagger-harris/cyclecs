#ifndef CLS_COMPONENTS_H
#define CLS_COMPONENTS_H

#include <cglm/types.h>
#include <cls/util/types.h>
#include <stdbool.h>
#include <stdio.h>

#define ECS_COMPONENT_UI_TEXT_MAX 128

struct group {
    u32 grp_id;
    u32 user_id;
};

struct renderable {
    vec2 uv_offset;
    vec2 uv_scale;
    ivec4 tint;
    u32 mesh_id;
    u32 shader_id;
    u32 texture_id;
    float opacity;
    bool visible;
    bool transparent;
};

struct transform {
    vec3 origin;
    vec3 pos;
    vec3 scale;
    vec3 rot_axis;
    float rot_angle;
};

struct ui_base {
    bool dirty;
    bool interactable;
};

struct ui_button {
    bool hovering;
    bool pressed;
    bool released;
    bool down;
};

struct ui_button_group {
    u8 _;
};

struct ui_label {
    char text[ECS_COMPONENT_UI_TEXT_MAX];
    int font_size;
    u32 font_id;
    bool font;
};

struct ui_label_group {
    u8 _;
};

struct ui_progress_bar {
    float value;
    float max_value;
};

#endif // CLS_COMPONENTS_H
