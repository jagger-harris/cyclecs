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

int gl_shader_init(struct shader *s, const struct shader_info *info);
void gl_shader_destroy(struct shader *s);
int gl_shader_use(const struct shader *s);
int gl_shader_set_bool(GLuint s, const char *id, GLboolean value);
int gl_shader_set_int(GLuint s, const char *id, GLint value);
int gl_shader_set_float(GLuint s, const char *id, GLfloat value);
int gl_shader_set_mat4(GLuint s, const char *id, mat4 *value);
int gl_shader_set_vec2(GLuint s, const char *id, vec2 *value);
int gl_shader_set_vec3(GLuint s, const char *id, vec3 *value);
int gl_shader_set_vec4(GLuint s, const char *id, vec4 *value);

#endif // GFX_GL_SHADER_H
