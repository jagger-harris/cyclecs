#include "core/app/assets.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/io/fascii.h"
#include "core/io/fimage.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include <stddef.h>
#include <string.h>

#define ASSETS_START_CAPACITY 16

#define ASSETS_DEFAULT_PATH "data/base/"
#define ASSETS_SHADER_PATH ASSETS_DEFAULT_PATH "gfx/shaders/"
#define ASSETS_TEXTURE_PATH ASSETS_DEFAULT_PATH "gfx/textures/"

static void assets_shader_add(struct assets *in, const char *path) {
    const char *vert_src = NULL;
    const char *frag_src = NULL;

    if (!in || !path)
        return;

    char shader_path[GLOBALS_PATH_MAX] = {0};
    snprintf(shader_path, GLOBALS_PATH_MAX, "%s", path);

    unsigned int found = false;
    table_find_cpy(&found, &in->shaders, shader_path);
    if (found) {
        LOGGER_LOG(LOGGER_INFO, "%s", "Duplicate shader use in assets");
        return;
    }

    char vert_path[GLOBALS_PATH_MAX] = {0};
    char frag_path[GLOBALS_PATH_MAX] = {0};
    strcat(vert_path, ASSETS_SHADER_PATH);
    strcat(vert_path, shader_path);
    strcat(vert_path, ".vert");
    strcat(frag_path, ASSETS_SHADER_PATH);
    strcat(frag_path, shader_path);
    strcat(frag_path, ".frag");

    int status = CORE_SUCCESS;
    status = fascii_init(&vert_src, vert_path);
    if (status) {
        LOGGER_LOG(LOGGER_WARN, "Reading vertex shader failed (%s)", vert_path);
        goto cleanup;
    }

    status = fascii_init(&frag_src, frag_path);
    if (status) {
        LOGGER_LOG(LOGGER_WARN, "Reading fragment shader failed (%s)",
                   frag_path);
        goto cleanup;
    }

    GLuint new_shader = 0;
    status = gl_shader_init(&new_shader, vert_src, frag_src);
    if (status) {
        LOGGER_LOG(LOGGER_WARN, "%s", "Init gl shader failed");
        goto cleanup;
    }

    fascii_destroy(vert_src);
    fascii_destroy(frag_src);

    status = table_insert(&in->shaders, path, &new_shader);
    if (status)
        goto cleanup;

    return;

cleanup:
    if (vert_src)
        fascii_destroy(vert_src);

    if (frag_src)
        fascii_destroy(frag_src);
}

static void assets_texture_add(struct assets *in, const char *path) {

    if (!in || !path)
        return;

    char texture_path[GLOBALS_PATH_MAX] = {0};
    snprintf(texture_path, GLOBALS_PATH_MAX, "%s", path);

    unsigned int found = 0;
    table_find_cpy(&found, &in->textures, texture_path);

    if (found) {
        LOGGER_LOG(LOGGER_INFO, "%s", "Duplicate texture use in assets");
        return;
    }

    char img_path[GLOBALS_PATH_MAX] = {0};
    strcat(img_path, ASSETS_TEXTURE_PATH);
    strcat(img_path, texture_path);

    struct fimage img = {0};
    int status = CORE_SUCCESS;
    status = fimage_init(&img, img_path);
    if (status) {
        LOGGER_LOG(LOGGER_WARN, "Reading image file failed (%s)", img_path);
        goto cleanup;
    }

    int width = img.width;
    int height = img.height;
    int channels = img.channels;
    unsigned char *data = img.data;

    // TODO: Support multiple apis
    GLuint new_texture = 0;
    status = gl_texture2d_init(&new_texture, data, width, height, channels);
    if (status) {
        LOGGER_LOG(LOGGER_WARN, "%s", "Init gl texture2d failed");
        goto cleanup;
    }

    fimage_destroy(&img);

    status = table_insert(&in->textures, path, &new_texture);
    if (status)
        goto cleanup;

    return;

cleanup:
    fimage_destroy(&img);
}

int assets_init(struct assets *in) {
    if (!in)
        return CORE_NULLPTR;

    int status = CORE_SUCCESS;
    status = table_init(&in->materials, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                        sizeof(struct material));
    if (status)
        goto cleanup;

    status = table_init(&in->meshes, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                        sizeof(struct gl_mesh));
    if (status)
        goto cleanup;

    status = table_init(&in->shaders, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                        sizeof(GLuint));
    if (status)
        goto cleanup;

    status = table_init(&in->textures, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                        sizeof(GLuint));
    if (status)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    assets_destroy(in);
    return status;
}

void assets_destroy(struct assets *in) {
    if (!in)
        return;

    table_destroy(&in->materials);
    table_destroy(&in->meshes);
    table_destroy(&in->shaders);
    table_destroy(&in->textures);
}

int assets_material_add(struct assets *in, const char *id,
                        const char *shader_path, const char *texture_path) {
    if (!in)
        return CORE_NULLPTR;

    struct material mat = {0};
    char final_shader_path[GLOBALS_PATH_MAX] = {0};
    char final_texture_path[GLOBALS_PATH_MAX] = {0};

    if (shader_path) {
        snprintf(final_shader_path, GLOBALS_PATH_MAX, "%s", shader_path);
        assets_shader_add(in, final_shader_path);
        snprintf(mat.shader_path, GLOBALS_PATH_MAX, "%s", final_shader_path);
    }

    if (texture_path) {
        snprintf(final_texture_path, GLOBALS_PATH_MAX, "%s", texture_path);
        assets_texture_add(in, final_texture_path);
        snprintf(mat.texture_path, GLOBALS_PATH_MAX, "%s", final_texture_path);
    }

    char mat_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mat_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_insert(&in->materials, mat_id, &mat);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_material_remove(struct assets *in, const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char mat_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mat_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_remove(NULL, &in->materials, mat_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_material_get(struct material **out, const struct assets *in,
                        const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char mat_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mat_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_find((void *)out, &in->materials, mat_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_mesh_add(struct assets *in, const char *id,
                    const struct vertex *vertices, size_t vertex_count,
                    unsigned int *indices, size_t index_count) {
    if (!in || !id)
        return CORE_NULLPTR;

    struct gl_mesh new_mesh = {0};
    int status = CORE_SUCCESS;
    status =
        gl_mesh_init(&new_mesh, vertices, vertex_count, indices, index_count);
    if (status)
        return status;

    char mesh_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mesh_id, GLOBALS_PATH_MAX, "%s", id);

    status = table_insert(&in->meshes, mesh_id, &new_mesh);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_mesh_remove(struct assets *in, const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char mesh_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mesh_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_remove(NULL, &in->meshes, mesh_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                    const char *id) {

    if (!in || !id)
        return CORE_NULLPTR;

    char mesh_id[GLOBALS_PATH_MAX] = {0};
    snprintf(mesh_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_find((void *)out, &in->meshes, mesh_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_shader_get(unsigned int *out, const struct assets *in,
                      const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char shader_id[GLOBALS_PATH_MAX] = {0};
    snprintf(shader_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_find_cpy(out, &in->shaders, shader_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}

int assets_texture_get(unsigned int *out, const struct assets *in,
                       const char *id) {
    if (!in || !id)
        return CORE_NULLPTR;

    char texture_id[GLOBALS_PATH_MAX] = {0};
    snprintf(texture_id, GLOBALS_PATH_MAX, "%s", id);

    int status = CORE_SUCCESS;
    status = table_find_cpy(out, &in->textures, texture_id);
    if (status)
        return status;

    return CORE_SUCCESS;
}
