/**
 * @file cls/gfx/gl/mesh.c
 * @brief OpenGL mesh for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/gfx/gl/mesh.h
 */

#include "cls/util/logger.h"
#include <cglm/types.h>
#include <cls/gfx/gl/mesh.h>
#include <cls/util/types.h>
#include <stddef.h>
#include <string.h>

cls_error cls_gl_mesh_init(struct cls_gl_mesh *mesh,
                           const struct cls_vertex *vertices,
                           size_t vertex_count, const unsigned int *indices,
                           size_t index_count) {
    if (!mesh || !vertices || !indices)
        return CLS_NULLPTR;

    mesh->index_count = (GLsizei)index_count;

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertex_count * sizeof(struct cls_vertex)),
                 vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(index_count * sizeof(u32)), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct cls_vertex),
                          (const void *)offsetof(struct cls_vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct cls_vertex),
                          (const void *)offsetof(struct cls_vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct cls_vertex),
                          (const void *)offsetof(struct cls_vertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    return CLS_SUCCESS;
}

void cls_gl_mesh_destroy(struct cls_gl_mesh *mesh) {
    if (!mesh)
        return;

    if (mesh->instance_vbo)
        glDeleteBuffers(1, &mesh->instance_vbo);

    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}

cls_error cls_gl_mesh_draw(const struct cls_gl_mesh *mesh) {
    if (!mesh)
        return CLS_NULLPTR;

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    return CLS_SUCCESS;
}

cls_error
cls_gl_mesh_draw_instanced(struct cls_gl_mesh *mesh,
                           struct cls_gl_mesh_instance_data *instances,
                           GLsizei instance_count) {
    if (!mesh || !instances)
        return CLS_NULLPTR;

    if (instance_count == 0)
        return CLS_INVALID_ARG;

    if (mesh->instance_vbo == 0) {
        glGenBuffers(1, &mesh->instance_vbo);
        glBindVertexArray(mesh->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            (GLsizeiptr)((GLsizeiptr)instance_count *
                         (GLsizeiptr)sizeof(struct cls_gl_mesh_instance_data)),
            NULL, GL_DYNAMIC_DRAW);
        mesh->instance_capacity =
            (GLsizeiptr)instance_count *
            (GLsizeiptr)sizeof(struct cls_gl_mesh_instance_data);

        GLsizei stride = sizeof(struct cls_gl_mesh_instance_data);

        size_t offset_mvp = offsetof(struct cls_gl_mesh_instance_data, mvp);
        for (GLuint i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(
                3 + i, 4, GL_FLOAT, GL_FALSE, stride,
                (const void *)(offset_mvp + sizeof(vec4) * i));
            glVertexAttribDivisor(3 + i, 1);
        }

        size_t offset_tint = offsetof(struct cls_gl_mesh_instance_data, tint);
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_tint);
        glVertexAttribDivisor(7, 1);

        size_t offset_uv_offset =
            offsetof(struct cls_gl_mesh_instance_data, uv_offset);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_uv_offset);
        glVertexAttribDivisor(8, 1);

        size_t offset_uv_scale =
            offsetof(struct cls_gl_mesh_instance_data, uv_scale);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 2, GL_FLOAT, GL_FALSE, stride,
                              (const void *)offset_uv_scale);
        glVertexAttribDivisor(9, 1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLsizeiptr size = (GLsizeiptr)instance_count *
                      (GLsizeiptr)sizeof(struct cls_gl_mesh_instance_data);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo);

    // Grow buffer if needed
    if (size > (GLsizeiptr)mesh->instance_capacity) {
        glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
        mesh->instance_capacity = size;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, size, instances);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0,
                            instance_count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return CLS_SUCCESS;
}
