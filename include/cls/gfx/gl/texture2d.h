/**
 * @file cls/gfx/gl/texture2d.h
 * @brief OpenGL texture2d for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/gfx/gl/texture2d.c
 */

#ifndef CLS_GL_TEXTURE2D_H
#define CLS_GL_TEXTURE2D_H

#include <cls/util/error.h>
#include <glad/gl.h>

/* Forward declarations. */
struct cls_texture2d;
struct cls_texture2d_info;

/**
 * @defgroup gl_texture2d OpenGL texture2d.
 * @ingroup gfx
 * @brief OpenGL texture2d.
 * @{
 */

/**
 * @struct cls_gl_texture2d
 * @brief OpenGL 2d texture.
 */
struct cls_gl_texture2d {
    GLuint id;
};

/**
 * @brief Creates a 2D texture.
 *
 * Creates and initializes an OpenGL texture using the provided texture
 * information.
 *
 * @param[out] tex  Texture.
 * @param[in]  info Texture information.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `tex` or `info` is NULL.
 * @retval CLS_GL      If creating the texture fails.
 *
 * @note The texture is unbound after creation.
 */
cls_error cls_gl_texture2d_init(struct cls_texture2d *tex,
                                struct cls_texture2d_info *info);

/**
 * @brief Destroys a 2D texture.
 *
 * Releases the OpenGL texture.
 *
 * @param[in] in Texture to destroy.
 */
void cls_gl_texture2d_destroy(struct cls_texture2d *in);

/**
 * @brief Uses a 2D texture.
 *
 * Binds the texture for rendering.
 *
 * @param[in] in Texture.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_INVALID_ARG If the texture is invalid.
 */
cls_error cls_gl_texture2d_use(const struct cls_texture2d *in);

/** @} */

#endif // CLS_GL_TEXTURE2D_H
