#include "core/gfx/gl/mesh.h"
#include "core/util/logger.h"

err gl_mesh_init(struct gl_mesh *out, const struct vertex *vertices,
                 size_t vertex_count, unsigned int *indices,
                 size_t index_count) {
    err status = CORE_SUCCESS;

    if (!out) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (!out) {
        status = CORE_OUT_OF_MEMORY;
        goto err;
    }

    out->index_count = index_count;

    glGenVertexArrays(1, &out->vao);
    glGenBuffers(1, &out->vbo);
    glGenBuffers(1, &out->ebo);

    glBindVertexArray(out->vao);

    glBindBuffer(GL_ARRAY_BUFFER, out->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(struct vertex),
                 vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint32_t),
                 indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (void *)offsetof(struct vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (void *)offsetof(struct vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
                          (void *)offsetof(struct vertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init gl mesh failed");
    return status;
}

void gl_mesh_destroy(struct gl_mesh *mesh) {
    if (!mesh)
        return;

    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    glDeleteBuffers(1, &mesh->ebo);
}

err gl_mesh_draw(const struct gl_mesh *mesh) {
    err status = CORE_SUCCESS;

    if (!mesh) {
        status = CORE_NULLPTR;
        goto err;
    }

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Drawing gl mesh failed");
    return status;
}
