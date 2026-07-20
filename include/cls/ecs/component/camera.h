/**
 * @file cls/ecs/component/camera.h
 * @brief Camera component helpers for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/component/camera.c
 */

#ifndef CLS_ECS_COMPONENT_CAMERA_H
#define CLS_ECS_COMPONENT_CAMERA_H

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/types.h>
#include <cls/util/error.h>
#include <cls/util/types.h>
#include <stdbool.h>

/* Forward declarations. */
struct cls_transform;

/**
 * @defgroup camera Camera
 * @ingroup ecs
 * @brief Camera component helpers.
 * @{
 */

/**
 * @enum cls_camera_type
 * @brief Type of camera projection.
 */
enum cls_camera_type { CLS_CAMERA_PERSP, CLS_CAMERA_ORTHO };

/**
 * @struct cls_camera_persp
 * @brief Perspective projection camera.
 */
struct cls_camera_persp {
    float fov;
    float aspect_ratio;
};

/**
 * @struct cls_camera_ortho
 * @brief Orthographic projection camera.
 */
struct cls_camera_ortho {
    float left;
    float right;
    float bottom;
    float top;
    bool y_down;
};

/**
 * @struct cls_camera
 * @brief Camera class.
 */
struct cls_camera {
    union {
        struct cls_camera_persp persp;
        struct cls_camera_ortho ortho;
    };
    mat4 view;
    mat4 projection;
    vec3 forward;
    enum cls_camera_type type;
    float zoom;
    float near_clip;
    float far_clip;
    float rot_angle;
    int layer;
    bool dirty;
};

/**
 * @brief Updates a camera.
 *
 * Updates the camera's view and projection matrices.
 *
 * @param[in] cam Camera.
 * @param[in] tf  Camera transform.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR     If `cam` or `tf` is NULL.
 * @retval CLS_INVALID_ARG If the camera type is invalid.
 */
cls_error cls_camera_update(struct cls_camera *cam, struct cls_transform *tf);

/**
 * @brief Resizes a camera.
 *
 * Updates the camera's projection for a new viewport size.
 *
 * @param[in] cam  Camera.
 * @param[in] size Viewport size.
 */
void cls_camera_resize(struct cls_camera *cam, ivec2 size);

/**
 * @brief Converts a screen position to world space.
 *
 * Converts a screen space position to world space.
 *
 * @param[out] pos           World space position.
 * @param[in]  cam           Camera.
 * @param[in]  cursor_pos    Screen space position.
 * @param[in]  viewport_size Viewport size.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR        If `pos` or `cam` is NULL.
 * @retval CLS_INVALID_ARG    If either viewport dimension is less than 1.
 * @retval CLS_DIVIDE_BY_ZERO If the homogeneous w component is 0.
 */
cls_error cls_camera_screen_to_world(vec2 pos, struct cls_camera *cam,
                                     const vec2 cursor_pos,
                                     const ivec2 viewport_size);

/** @} */

#endif // CLS_ECS_COMPONENT_CAMERA_H
