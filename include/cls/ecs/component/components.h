#ifndef CLS_COMPONENTS_H
#define CLS_COMPONENTS_H

#include <cglm/types.h>
#include <cls/util/types.h>
#include <stdbool.h>
#include <stdio.h>

#define CLS_UI_TEXT_MAX 128

#define CLS_COMP_CAMERA "camera"
#define CLS_COMP_CAMERA_ACTIVE "camera_active"
#define CLS_COMP_GROUP "group"
#define CLS_COMP_RENDERABLE "renderable"
#define CLS_COMP_TRANSFORM "transform"
#define CLS_COMP_UI "ui"
#define CLS_COMP_BUTTON "button"
#define CLS_COMP_BUTTON_GROUP "button_group"
#define CLS_COMP_LABEL "label"
#define CLS_COMP_LABEL_GROUP "label_group"
#define CLS_COMP_PROGRESS_BAR "progress_bar"

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

struct ui {
    bool dirty;
    bool interactable;
};

struct button {
    bool hovering;
    bool pressed;
    bool released;
    bool down;
};

struct button_group {
    u8 _;
};

struct label {
    char text[CLS_UI_TEXT_MAX];
    int font_size;
    u32 font_id;
    bool font;
};

struct label_group {
    u8 _;
};

struct progress_bar {
    float value;
    float max_value;
};

#endif // CLS_COMPONENTS_H
