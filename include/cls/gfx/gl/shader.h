/**
 * @file cls/gfx/gl/shader.h
 * @brief OpenGL shader for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/gfx/gl/shader.c
 */

#ifndef CLS_GL_SHADER_H
#define CLS_GL_SHADER_H

#include <cglm/cglm.h>
#include <cls/util/error.h>
#include <glad/gl.h>
#include <stdint.h>

/* Forward declarations. */
struct cls_shader;
struct cls_shader_info;

/**
 * @defgroup gl_shader OpenGL shader.
 * @ingroup gfx
 * @brief OpenGL shader.
 * @{
 */

/**
 * @struct cls_gl_shader
 * @brief OpenGL shader.
 */
struct cls_gl_shader {
    GLuint id;
};

/**
 * @brief Creates a shader.
 *
 * Compiles the vertex and fragment shaders and links them into a shader
 * program. Destroy the returned shader with cls_gl_shader_destroy().
 *
 * @param[out] s    Shader.
 * @param[in]  info Shader source.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `s`, `info`, `info->vert_src`, or `info->frag_src`
 *                     is NULL.
 * @retval CLS_GL      If linking the shader program fails.
 * @retval (error)     If shader compilation fails.
 */
cls_error cls_gl_shader_init(struct cls_shader *s,
                             const struct cls_shader_info *info);

/**
 * @brief Destroys a shader.
 *
 * Releases the shader program.
 *
 * @param[in] s Shader to destroy.
 */
void cls_gl_shader_destroy(struct cls_shader *s);

/**
 * @brief Uses a shader.
 *
 * Sets the shader program as the current OpenGL program.
 *
 * @param[in] s Shader.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `s` is NULL.
 * @retval CLS_INVALID_ARG If the shader program is invalid.
 */
cls_error cls_gl_shader_use(const struct cls_shader *s);

/**
 * @brief Sets a boolean uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Value.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_bool(GLuint s, const char *id, GLboolean value);

/**
 * @brief Sets an integer uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Value.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_int(GLuint s, const char *id, GLint value);

/**
 * @brief Sets a float uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Value.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR     If `id` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_float(GLuint s, const char *id, GLfloat value);

/**
 * @brief Sets a mat4 uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Matrix.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` or `value` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_mat4(GLuint s, const char *id, mat4 *value);

/**
 * @brief Sets a vec2 uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Vector.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` or `value` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_vec2(GLuint s, const char *id, vec2 *value);

/**
 * @brief Sets a vec3 uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Vector.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` or `value` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_vec3(GLuint s, const char *id, vec3 *value);

/**
 * @brief Sets a vec4 uniform.
 *
 * @param[in] s     Shader program.
 * @param[in] id    Uniform name.
 * @param[in] value Vector.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `id` or `value` is NULL.
 * @retval CLS_INVALID_ARG If `s` is invalid.
 */
cls_error cls_gl_shader_set_vec4(GLuint s, const char *id, vec4 *value);

/** @} */

#endif // CLS_GL_SHADER_H
