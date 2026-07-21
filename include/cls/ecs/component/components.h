/**
 * @file cls/ecs/component/components.h
 * @brief Default components for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_COMPONENTS_H
#define CLS_COMPONENTS_H

#include <cglm/types.h>
#include <cls/app/assets.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/ecs.h>
#include <cls/io/font.h>
#include <cls/util/types.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @defgroup components Components
 * @ingroup ecs
 * @brief Default ECS components.
 * @{
 */

enum {
    CLS_COMP_CAMERA = 0,
    CLS_COMP_CAMERA_ACTIVE,
    CLS_SINGLETON_CAMERA_ACTIVE,
    CLS_COMP_GROUP,
    CLS_COMP_RENDERABLE,
    CLS_COMP_TRANSFORM,
    CLS_COMP_UI,
    CLS_COMP_BUTTON,
    CLS_COMP_BUTTON_GROUP,
    CLS_COMP_LABEL,
    CLS_COMP_PROGRESS_BAR
};

enum {
    CLS_UI_TEXT_MAX = 128,
};

/**
 * @struct cls_group
 * @brief Group component.
 */
struct cls_group {
    u32 grp_id;
    u32 user_id;
};

/**
 * @struct cls_render_state
 * @brief Render state sub-component.
 */
struct cls_render_state {
    bool depth_test;
    bool depth_write;
    bool blending;
    int blend_src;
    int blend_dest;
};

/**
 * @struct cls_renderable
 * @brief Renderable component.
 */

struct cls_renderable {
    struct cls_render_state state;
    vec2 uv_offset;
    vec2 uv_scale;
    ivec4 tint;
    cls_gl_mesh_id mesh_id;
    cls_shader_id shader_id;
    cls_texture2d_id texture_id;
    float opacity;
    bool visible;
};

/**
 * @struct cls_transform
 * @brief Transform component.
 */
struct cls_transform {
    vec3 pos;
    vec3 scale;
    vec3 rot_axis;
    float rot_angle;
    bool dirty;
};

/**
 * @struct cls_singleton_camera_active
 * @brief Singleton active camera component.
 */
struct cls_singleton_camera_active {
    cls_entity entity;
};

/**
 * @struct cls_camera_active
 * @brief Active camera component.
 */
struct cls_camera_active {
    u8 _;
};

/**
 * @struct cls_ui
 * @brief UI component.
 */
struct cls_ui {
    bool dirty;
    bool interactable;
};

/**
 * @struct cls_button
 * @brief Button component.
 */
struct cls_button {
    bool hovering;
    bool pressed;
    bool released;
    bool down;
};

/**
 * @struct cls_button_group
 * @brief Button group component.
 */
struct cls_button_group {
    u8 _;
};

/**
 * @struct cls_label
 * @brief Label component.
 */
struct cls_label {
    char text[CLS_UI_TEXT_MAX];
    ivec4 tint;
    int font_size;
    u32 font_id;
    bool visible;
    bool dirty;
};

/**
 * @struct cls_progress_bar
 * @brief Progress bar component.
 */
struct cls_progress_bar {
    float value;
    float max_value;
};

/** @} */

#endif // CLS_COMPONENTS_H
