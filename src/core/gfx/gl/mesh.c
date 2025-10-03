#include "core/gfx/gl/mesh.h"
#include "core/util/error.h"
#include "core/util/types.h"
#include <cglm/types.h>
#include <stddef.h>

int gl_mesh_init(struct gl_mesh *out, const struct vertex *vertices,
                 size_t vertex_count, const unsigned int *indices,
                 size_t index_count) {
    if (!out || !vertices || !indices)
        return CORE_NULLPTR;

    out->index_count = (GLsizei)index_count;

    glGenVertexArrays(1, &out->vao);
    glGenBuffers(1, &out->vbo);
    glGenBuffers(1, &out->ebo);

    glBindVertexArray(out->vao);

    glBindBuffer(GL_ARRAY_BUFFER, out->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertex_count * sizeof(struct vertex)), vertices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(index_count * sizeof(u32)), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (const void *)offsetof(struct vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (const void *)offsetof(struct vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (const void *)offsetof(struct vertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    return CORE_SUCCESS;
}

void gl_mesh_destroy(struct gl_mesh *mesh) {
    if (!mesh)
        return;

    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}

int gl_mesh_draw(const struct gl_mesh *mesh) {
    if (!mesh)
        return CORE_NULLPTR;

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    return CORE_SUCCESS;
}

int gl_mesh_draw_instanced(struct gl_mesh *mesh,
                           struct gl_mesh_instance_data *instances,
                           GLsizei instance_count) {
    if (!mesh || instance_count == 0)
        return CORE_NULLPTR;

    if (mesh->instance_vbo == 0) {
        glGenBuffers(1, &mesh->instance_vbo);
        glBindVertexArray(mesh->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            (GLsizeiptr)(instance_count * sizeof(struct gl_mesh_instance_data)),
            NULL, GL_DYNAMIC_DRAW);

        GLsizei stride = sizeof(struct gl_mesh_instance_data);
        size_t offset_mvp = offsetof(struct gl_mesh_instance_data, mvp);
        for (int i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(
                3 + i, 4, GL_FLOAT, GL_FALSE, stride,
                (const void *)(offset_mvp + sizeof(vec4) * i));
            glVertexAttribDivisor(3 + i, 1);
        }

        size_t offset_tint = offsetof(struct gl_mesh_instance_data, tint);
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_tint);
        glVertexAttribDivisor(7, 1);

        size_t offset_uv_offset =
            offsetof(struct gl_mesh_instance_data, uv_offset);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_uv_offset);
        glVertexAttribDivisor(8, 1);

        size_t offset_uv_scale =
            offsetof(struct gl_mesh_instance_data, uv_scale);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 2, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_uv_scale);
        glVertexAttribDivisor(9, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo);
    GLsizeiptr size =
        (GLsizeiptr)(instance_count * sizeof(struct gl_mesh_instance_data));

    if (size > mesh->instance_capacity) {
        glBufferData(GL_ARRAY_BUFFER, size, instances, GL_DYNAMIC_DRAW);
        mesh->instance_capacity = size;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, instances);
    }

    glBindVertexArray(mesh->vao);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0,
                            instance_count);
    return CORE_SUCCESS;
}
