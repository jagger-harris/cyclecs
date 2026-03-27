#ifndef CLS_GL_MESH_H
#define CLS_GL_MESH_H

#include <cglm/cglm.h>
#include <glad/gl.h>

struct cls_vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct cls_gl_mesh {
    GLsizei index_count;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint instance_vbo;
    GLsizeiptr instance_capacity;
};

struct cls_gl_mesh_instance_data {
    mat4 mvp;
    vec4 tint;
    vec2 uv_offset;
    vec2 uv_scale;
};

int cls_gl_mesh_init(struct cls_gl_mesh *mesh,
                     const struct cls_vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count);
void cls_gl_mesh_destroy(struct cls_gl_mesh *mesh);
int cls_gl_mesh_draw(const struct cls_gl_mesh *mesh);
int cls_gl_mesh_draw_instanced(struct cls_gl_mesh *mesh,
                               struct cls_gl_mesh_instance_data *instances,
                               GLsizei instance_count);

#endif // CLS_GL_MESH_H
