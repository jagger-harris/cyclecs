#ifndef GFX_GL_MESH_H
#define GFX_GL_MESH_H

#include <cglm/cglm.h>
#include <glad/gl.h>
#include <stdint.h>

struct vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct gl_mesh {
    GLsizei index_count;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
};

int gl_mesh_init(struct gl_mesh *out, const struct vertex *vertices,
                 size_t vertex_count, unsigned int *indices,
                 size_t index_count);
void gl_mesh_destroy(struct gl_mesh *mesh);
int gl_mesh_draw(const struct gl_mesh *mesh);

#endif // GFX_GL_MESH_H
