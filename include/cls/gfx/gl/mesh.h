#ifndef CLS_GL_MESH_H
#define CLS_GL_MESH_H

#include <cglm/cglm.h>
#include <cls/ecs/component/components.h>
#include <glad/gl.h>

/**
 * @struct cls_vertex
 * @brief Mesh vertex.
 */
struct cls_vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

/**
 * @struct cls_gl_mesh
 * @brief OpenGL mesh.
 */
struct cls_gl_mesh {
    GLsizei index_count;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint instance_vbo;
    GLsizeiptr instance_capacity;
};

/**
 * @struct cls_gl_mesh_instance_data
 * @brief Mesh instance data.
 */
struct cls_gl_mesh_instance_data {
    mat4 mvp;
    vec4 tint;
    vec2 uv_offset;
    vec2 uv_scale;
};

/**
 * @brief Creates a mesh.
 *
 * Uploads vertex and index data to the GPU and initializes the mesh buffers.
 * Destroy the returned mesh with cls_gl_mesh_destroy().
 *
 * @param[out] mesh         Mesh.
 * @param[in]  vertices     Vertex data.
 * @param[in]  vertex_count Number of vertices.
 * @param[in]  indices      Index data.
 * @param[in]  index_count  Number of indices.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `mesh`, `vertices`, or `indices` is NULL.
 */
cls_error cls_gl_mesh_init(struct cls_gl_mesh *mesh,
                           const struct cls_vertex *vertices,
                           size_t vertex_count, const unsigned int *indices,
                           size_t index_count);

/**
 * @brief Destroys a mesh.
 *
 * Releases the mesh GPU buffers.
 *
 * @param[in] mesh Mesh to destroy.
 */
void cls_gl_mesh_destroy(struct cls_gl_mesh *mesh);

/**
 * @brief Draws a mesh.
 *
 * @param[in] mesh Mesh.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `mesh` is NULL.
 */
cls_error cls_gl_mesh_draw(const struct cls_gl_mesh *mesh);

/**
 * @brief Draws mesh instances.
 *
 * Uploads instance data and draws multiple mesh instances in one draw call.
 *
 * @param[in] mesh           Mesh.
 * @param[in] instances      Instance data.
 * @param[in] instance_count Number of instances.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR     If `mesh` is NULL.
 * @retval CLS_INVALID_ARG If `instance_count` is zero.
 */
cls_error
cls_gl_mesh_draw_instanced(struct cls_gl_mesh *mesh,
                           struct cls_gl_mesh_instance_data *instances,
                           GLsizei instance_count);

#endif // CLS_GL_MESH_H
