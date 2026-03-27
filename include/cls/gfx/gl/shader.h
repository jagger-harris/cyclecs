#ifndef CLS_GL_SHADER_H
#define CLS_GL_SHADER_H

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <stdint.h>

struct cls_shader;
struct cls_shader_info;

struct cls_gl_shader {
    GLuint id;
};

int cls_gl_shader_init(struct cls_shader *s,
                       const struct cls_shader_info *info);
void cls_gl_shader_destroy(struct cls_shader *s);
int cls_gl_shader_use(const struct cls_shader *s);
int cls_gl_shader_set_bool(GLuint s, const char *id, GLboolean value);
int cls_gl_shader_set_int(GLuint s, const char *id, GLint value);
int cls_gl_shader_set_float(GLuint s, const char *id, GLfloat value);
int cls_gl_shader_set_mat4(GLuint s, const char *id, mat4 *value);
int cls_gl_shader_set_vec2(GLuint s, const char *id, vec2 *value);
int cls_gl_shader_set_vec3(GLuint s, const char *id, vec3 *value);
int cls_gl_shader_set_vec4(GLuint s, const char *id, vec4 *value);

#endif // CLS_GL_SHADER_H
