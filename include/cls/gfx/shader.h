/**
 * @file cls/gfx/shader.h
 * @brief Generic shader for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_SHADER_H
#define CLS_SHADER_H

#include <cls/gfx/gl/shader.h>
#include <cls/util/types.h>
#include <stdbool.h>

/**
 * @defgroup shader Shader.
 * @ingroup gfx
 * @brief Universal shader.
 * @{
 */

/**
 * @struct cls_shader
 * @brief Universal shader.
 */
struct cls_shader {
    union {
        struct cls_gl_shader gl;
    };
};

/**
 * @struct cls_shader_info
 * @brief Universal shader info.
 */
struct cls_shader_info {
    const char *vert_src;
    const char *frag_src;
};

/** @} */

#endif // CLS_SHADER_H
