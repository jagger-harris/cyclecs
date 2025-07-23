#include "core/app/assets.h"
#include "core/gfx/gl/mesh.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/io/fimg.h"
#include "core/io/ftxt.h"
#include "core/util/err.h"
#include "core/util/logger.h"
#include <stddef.h>
#include <string.h>

#define STR_MAX 512
#define DARRAY_START_SIZE 16

#define ASSETS_DEFAULT_PATH "data/base/"
#define ASSETS_SHADER_PATH ASSETS_DEFAULT_PATH "gfx/shaders/"
#define ASSETS_TEXTURE_PATH ASSETS_DEFAULT_PATH "gfx/textures/"

static void assets_shader_add(struct assets *in, const char *path) {
    err status = CORE_SUCCESS;
    const char *vert_src = NULL;
    const char *frag_src = NULL;

    if (!in || !path) {
        status = CORE_NULLPTR;
        goto err;
    }

    char shader_path[ASSETS_STR_MAX] = {0};
    snprintf(shader_path, ASSETS_STR_MAX, "%s", path);

    unsigned int found = 0;
    table_find(&found, &in->shaders, shader_path);

    if (found)
        return;

    char vert_path[ASSETS_STR_MAX] = {0};
    char frag_path[ASSETS_STR_MAX] = {0};
    strcat(vert_path, ASSETS_SHADER_PATH);
    strcat(vert_path, shader_path);
    strcat(vert_path, ".vert");
    strcat(frag_path, ASSETS_SHADER_PATH);
    strcat(frag_path, shader_path);
    strcat(frag_path, ".frag");

    status = ftxt_init(&vert_src, vert_path);
    if (status)
        goto err;

    status = ftxt_init(&frag_src, frag_path);
    if (status)
        goto err;

    GLuint new_shader = 0;
    status = gl_shader_init(&new_shader, vert_src, frag_src);
    if (status)
        goto err;

    ftxt_destroy(vert_src);
    ftxt_destroy(frag_src);

    status = table_insert(&in->shaders, path, &new_shader);
    if (status)
        goto err;

    return;

err:
    if (vert_src)
        ftxt_destroy(vert_src);

    if (frag_src)
        ftxt_destroy(frag_src);

    logger_log_err(LOGGER_ERR, status, "Adding new shader failed");
}

static void assets_texture_add(struct assets *in, const char *path) {
    err status = CORE_SUCCESS;
    struct fimg img = {0};

    if (!in || !path) {
        status = CORE_NULLPTR;
        goto err;
    }

    char texture_path[ASSETS_STR_MAX] = {0};
    snprintf(texture_path, ASSETS_STR_MAX, "%s", path);

    unsigned int found = 0;
    table_find(&found, &in->textures, texture_path);

    if (found)
        return;

    char img_path[STR_MAX] = {0};
    strcat(img_path, ASSETS_TEXTURE_PATH);
    strcat(img_path, texture_path);

    status = fimg_init(&img, img_path);
    if (status)
        goto err;

    int width = img.width;
    int height = img.height;
    int channels = img.channels;
    unsigned char *data = img.data;

    // TODO: Support multiple apis
    GLuint new_texture = 0;
    status = gl_texture2d_init(&new_texture, data, width, height, channels);
    if (status)
        goto err;

    fimg_destroy(&img);

    status = table_insert(&in->textures, path, &new_texture);
    if (status)
        goto err;

    return;

err:
    fimg_destroy(&img);
    logger_log_err(LOGGER_ERR, status, "Adding new texture failed");
}

err assets_init(struct assets *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    // TODO: Support multiple apis
    status = table_init(&in->materials, DARRAY_START_SIZE,
                        sizeof(char[ASSETS_STR_MAX]), sizeof(struct material));
    if (status)
        goto err;

    status = table_init(&in->meshes, DARRAY_START_SIZE,
                        sizeof(char[ASSETS_STR_MAX]), sizeof(struct gl_mesh));
    if (status)
        goto err;

    status = table_init(&in->shaders, DARRAY_START_SIZE,
                        sizeof(char[ASSETS_STR_MAX]), sizeof(GLuint));
    if (status)
        goto err;

    status = table_init(&in->textures, DARRAY_START_SIZE,
                        sizeof(char[ASSETS_STR_MAX]), sizeof(GLuint));
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init assets failed");
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

void assets_material_add(struct assets *in, const char *id,
                         const char *shader_path, const char *texture_path) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct material mat = {0};
    char final_shader_path[ASSETS_STR_MAX] = {0};
    char final_texture_path[ASSETS_STR_MAX] = {0};

    if (shader_path) {
        snprintf(final_shader_path, ASSETS_STR_MAX, "%s", shader_path);
        assets_shader_add(in, final_shader_path);
        snprintf(mat.shader_path, ASSETS_STR_MAX, "%s", final_shader_path);
    }

    if (texture_path) {
        snprintf(final_texture_path, ASSETS_STR_MAX, "%s", texture_path);
        assets_texture_add(in, final_texture_path);
        snprintf(mat.texture_path, ASSETS_STR_MAX, "%s", final_texture_path);
    }

    char mat_id[ASSETS_STR_MAX] = {0};
    snprintf(mat_id, ASSETS_STR_MAX, "%s", id);

    status = table_insert(&in->materials, mat_id, &mat);
    if (status)
        goto err;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Adding new material failed");
}

void assets_material_remove(struct assets *in, const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char mat_id[ASSETS_STR_MAX] = {0};
    snprintf(mat_id, ASSETS_STR_MAX, "%s", id);

    status = table_remove(NULL, &in->materials, mat_id);
    if (status)
        goto err;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Removing material failed");
}

void assets_material_get(struct material **out, const struct assets *in,
                         const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char mat_id[ASSETS_STR_MAX] = {0};
    snprintf(mat_id, ASSETS_STR_MAX, "%s", id);
    table_find_ptr((void *)out, &in->materials, mat_id);
    return;

err:
    logger_log_err(LOGGER_ERR, status, "Getting material failed");
}

void assets_mesh_add(struct assets *in, const char *id,
                     const struct vertex *vertices, size_t vertex_count,
                     unsigned int *indices, size_t index_count) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct gl_mesh new_mesh = {0};
    status =
        gl_mesh_init(&new_mesh, vertices, vertex_count, indices, index_count);
    if (status)
        goto err;

    char mesh_id[ASSETS_STR_MAX] = {0};
    snprintf(mesh_id, ASSETS_STR_MAX, "%s", id);

    status = table_insert(&in->meshes, mesh_id, &new_mesh);
    if (status)
        goto err;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Adding new mesh failed");
}

void assets_mesh_remove(struct assets *in, const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char mesh_id[ASSETS_STR_MAX] = {0};
    snprintf(mesh_id, ASSETS_STR_MAX, "%s", id);

    status = table_remove(NULL, &in->meshes, mesh_id);
    if (status)
        goto err;

    return;

err:
    logger_log_err(LOGGER_ERR, status, "Removing mesh failed");
}

void assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                     const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char mesh_id[ASSETS_STR_MAX] = {0};
    snprintf(mesh_id, ASSETS_STR_MAX, "%s", id);
    table_find_ptr((void *)out, &in->meshes, mesh_id);
    return;

err:
    logger_log_err(LOGGER_ERR, status, "Getting mesh failed");
}

void assets_shader_get(unsigned int *out, const struct assets *in,
                       const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char shader_id[ASSETS_STR_MAX] = {0};
    snprintf(shader_id, ASSETS_STR_MAX, "%s", id);
    table_find((void *)out, &in->shaders, shader_id);
    return;

err:
    logger_log_err(LOGGER_ERR, status, "Getting shader failed");
}

void assets_texture_get(unsigned int *out, const struct assets *in,
                        const char *id) {
    err status = CORE_SUCCESS;

    if (!in || !id) {
        status = CORE_NULLPTR;
        goto err;
    }

    char texture_id[ASSETS_STR_MAX] = {0};
    snprintf(texture_id, ASSETS_STR_MAX, "%s", id);
    table_find((void *)out, &in->textures, texture_id);
    return;

err:
    logger_log_err(LOGGER_ERR, status, "Getting texture failed");
}
