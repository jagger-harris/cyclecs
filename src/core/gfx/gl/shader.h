#ifndef GFX_GL_SHADER_H
#define GFX_GL_SHADER_H

#include "core/util/err.h"
#include <cglm/cglm.h>
#include <glad/gl.h>
#include <stdint.h>

err gl_shader_new(GLuint *out, const char *vert_src, const char *frag_src);
err gl_shader_delete(GLuint shader);
err gl_shader_use(GLuint shader);
err gl_shader_set_bool(GLuint shader, const char *name, GLboolean data);
err gl_shader_set_int(GLuint shader, const char *name, GLint data);
err gl_shader_set_float(GLuint shader, const char *name, GLfloat data);
err gl_shader_set_two_float(GLuint shader, const char *name, GLfloat data1,
                            GLfloat data2);
err gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data);
err gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data);

#endif /* GFX_GL_SHADER_H */
