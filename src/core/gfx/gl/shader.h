#ifndef GFX_GL_SHADER_H
#define GFX_GL_SHADER_H

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <stdint.h>

struct shader;
struct shader_info;

struct gl_shader {
    GLuint id;
};

int gl_shader_init(struct shader *out, const struct shader_info *info);
void gl_shader_destroy(struct shader *in);
int gl_shader_use(const struct shader *in);
int gl_shader_set_bool(GLuint shader, const char *name, GLboolean data);
int gl_shader_set_int(GLuint shader, const char *name, GLint data);
int gl_shader_set_float(GLuint shader, const char *name, GLfloat data);
int gl_shader_set_mat4(GLuint shader, const char *name, mat4 *data);
int gl_shader_set_vec2(GLuint shader, const char *name, vec2 *data);
int gl_shader_set_vec3(GLuint shader, const char *name, vec3 *data);
int gl_shader_set_vec4(GLuint shader, const char *name, vec4 *data);

#endif // GFX_GL_SHADER_H
