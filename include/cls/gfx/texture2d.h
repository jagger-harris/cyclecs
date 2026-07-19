/**
 * @file cls/gfx/texture2d.h
 * @brief Generic texture2d for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_TEXTURE2D_H
#define CLS_TEXTURE2D_H

#include <cls/gfx/gl/texture2d.h>
#include <cls/util/types.h>
#include <stdbool.h>

/**
 * @defgroup texture2d Texture2D.
 * @ingroup gfx
 * @brief Universal texture2d.
 * @{
 */

enum cls_texture2d_filter {
    CLS_TEXTURE_FILTER_LINEAR,
    CLS_TEXTURE_FILTER_NEAREST
};
enum cls_texture2d_wrap { CLS_TEXTURE_WRAP_CLAMP, CLS_TEXTURE_WRAP_REPEAT };

/**
 * @struct cls_texture2d
 * @brief Universal texture2d.
 */
struct cls_texture2d {
    union {
        struct cls_gl_texture2d gl;
    };
};

/**
 * @struct cls_texture2d_info
 * @brief Universal texture2d info.
 */
struct cls_texture2d_info {
    u8 *data;
    int width;
    int height;
    int channels;
    enum cls_texture2d_filter filter;
    enum cls_texture2d_wrap wrap;
};

/** @} */

#endif // CLS_TEXTURE2D_H
