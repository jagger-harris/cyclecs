#ifndef GFX_GL_SHADER_H
#define GFX_GL_SHADER_H

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <stdint.h>

int gl_shader_init(GLuint *out, const char *vert_src, const char *frag_src);
void gl_shader_destroy(GLuint shader);
int gl_shader_use(GLuint shader);
int gl_shader_set_bool(GLuint shader, const char *name, GLboolean data);
int gl_shader_set_int(GLuint shader, const char *name, GLint data);
int gl_shader_set_float(GLuint shader, const char *name, GLfloat data);
int gl_shader_set_two_float(GLuint shader, const char *name, GLfloat data1,
                            GLfloat data2);
int gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data);
int gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data);

#endif // GFX_GL_SHADER_H
