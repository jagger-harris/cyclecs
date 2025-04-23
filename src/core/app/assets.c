#include "core/app/assets.h"
#include "core/gfx/gl/shader.h"
#include "core/gfx/gl/texture2d.h"
#include "core/io/fimg.h"
#include "core/io/ftxt.h"
#include "core/util/array.h"
#include "core/util/logger.h"
#include "core/util/table.h"
#include <stddef.h>
#include <string.h>

#define STR_MAX 512
#define DARRAY_START_SIZE 16

#define ASSETS_DEFAULT_PATH "data/base/"
#define ASSETS_SHADER_PATH ASSETS_DEFAULT_PATH "gfx/shaders/"
#define ASSETS_TEXTURE_PATH ASSETS_DEFAULT_PATH "gfx/textures/"

struct assets {
    array *gl_materials;
    table *gl_shaders;
    table *gl_textures;
};

// static void assets_shader_remove(assets *in, char path[ASSETS_STR_MAX]) {
//     err err = CORE_SUCCESS;
//
//     if (!in) {
//         err = CORE_INVALID_NULLPTR;
//         goto err;
//     }
//
//     /* TODO: Support multiple apis */
//     GLuint id = 0;
//
//     err = table_remove(&id, &in->gl_shaders, path);
//     if (err)
//         goto err;
//
//     err = gl_shader_delete(id);
//     if (err)
//         goto err;
//
//     return;
//
// err:
//     logger_log(LOGGER_ERR, "Failed to remove shader from assets", err);
// }
//
// static void assets_texture_remove(assets *in, char path[ASSETS_STR_MAX]) {
//     err err = CORE_SUCCESS;
//
//     if (!in) {
//         err = CORE_INVALID_NULLPTR;
//         goto err;
//     }
//
//     /* TODO: Support multiple apis */
//     GLuint id = 0;
//
//     err = table_remove(&id, &in->gl_shaders, path);
//     if (err)
//         goto err;
//
//     err = gl_texture2d_delete(id);
//     if (err)
//         goto err;
//
//     return;
//
// err:
//     logger_log(LOGGER_ERR, "Failed to remove shader from assets", 0);
// }

static void assets_shader_add(assets *in, char path[ASSETS_STR_MAX]) {
    err err = CORE_SUCCESS;

    if (!in || !path) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    unsigned int found = 0;
    table_find(&found, in->gl_shaders, path);

    if (found) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    char vert_path[STR_MAX] = {0};
    char frag_path[STR_MAX] = {0};
    strcat(vert_path, ASSETS_SHADER_PATH);
    strcat(vert_path, path);
    strcat(vert_path, ".vert");
    strcat(frag_path, ASSETS_SHADER_PATH);
    strcat(frag_path, path);
    strcat(frag_path, ".frag");

    const char *vert_src = NULL;
    const char *frag_src = NULL;

    err = ftxt_new(&vert_src, vert_path);
    if (err)
        goto cleanup;

    err = ftxt_new(&frag_src, frag_path);
    if (err)
        goto cleanup;

    GLuint new_shader = 0;
    err = gl_shader_new(&new_shader, vert_src, frag_src);
    if (err)
        goto cleanup;

    err = ftxt_delete(vert_src);
    if (err)
        goto err;

    err = ftxt_delete(frag_src);
    if (err)
        goto err;

    err = table_insert(in->gl_shaders, path, &new_shader);
    if (err)
        goto err;

    return;

cleanup:
    if (vert_src)
        err = ftxt_delete(vert_src);

    if (frag_src)
        err = ftxt_delete(frag_src);

err:
    logger_log(LOGGER_ERR, "Failed to add new shader to assets", err);
}

static void assets_texture_add(assets *in, char path[ASSETS_STR_MAX]) {
    err err = CORE_SUCCESS;

    if (!in || !path) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    unsigned int found = 0;
    table_find(&found, in->gl_textures, path);

    if (found) {
        err = CORE_INVALID_ARGS;
        goto err;
    }

    char img_path[STR_MAX] = {0};
    strcat(img_path, ASSETS_TEXTURE_PATH);
    strcat(img_path, path);

    fimg *img = NULL;
    err = fimg_new(&img, img_path);
    if (err)
        goto err;

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *data = NULL;

    err = fimg_get_dims(&width, &height, img);
    if (err)
        goto cleanup;

    err = fimg_get_channels(&channels, img);
    if (err)
        goto cleanup;

    err = fimg_get_data(&data, img);
    if (err)
        goto cleanup;

    /* TODO: Support multiple apis */
    GLuint new_texture = 0;
    err = gl_texture2d_new(&new_texture, data, width, height, channels);
    if (err)
        goto cleanup;

    err = fimg_delete(img);
    if (err)
        goto err;

    err = table_insert(in->gl_textures, path, &new_texture);
    if (err)
        goto err;

    return;

cleanup:
    if (img)
        fimg_delete(img);

err:
    logger_log(LOGGER_ERR, "Failed to add new texture to assets", err);
}

err assets_new(assets **out, arena *mem) {
    err err = CORE_SUCCESS;

    if (!out || !mem) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = arena_alloc((void **)out, mem, sizeof(assets), _Alignof(assets));
    if (err)
        goto err;

    /* TODO: Support multiple apis */
    err = array_new(&(*out)->gl_materials, 16, sizeof(struct material));
    if (err)
        goto err;

    err = table_new(&(*out)->gl_shaders, DARRAY_START_SIZE,
                    sizeof(char[ASSETS_STR_MAX]), sizeof(GLuint));
    if (err)
        goto err;

    err = table_new(&(*out)->gl_textures, DARRAY_START_SIZE,
                    sizeof(char[ASSETS_STR_MAX]), sizeof(GLuint));
    if (err)
        goto err;

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to create new assets", err);
    assets_delete(*out);
    return err;
}

err assets_delete(assets *in) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    array_delete(in->gl_materials);
    table_delete(in->gl_shaders);
    table_delete(in->gl_textures);
    return err;

err:
    logger_log(LOGGER_ERR, "Failed to delete assets", err);
    return err;
}

void assets_material_add(assets *in, int id, const char *shader_path,
                         const char *texture_path) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    struct material material = {0};
    char final_shader_path[ASSETS_STR_MAX] = {0};
    char final_texture_path[ASSETS_STR_MAX] = {0};

    if (shader_path) {
        snprintf(final_shader_path, ASSETS_STR_MAX, "%s", shader_path);
        assets_shader_add(in, final_shader_path);
        snprintf(material.shader_path, ASSETS_STR_MAX, "%s", final_shader_path);
    }

    if (texture_path) {
        snprintf(final_texture_path, ASSETS_STR_MAX, "%s", texture_path);
        assets_texture_add(in, final_texture_path);
        snprintf(material.texture_path, ASSETS_STR_MAX, "%s",
                 final_shader_path);
    }

    err = array_insert(in->gl_materials, id, &material);
    if (err)
        goto err;

    return;

err:
    logger_log(LOGGER_ERR, "Failed to add new material to assets", err);
}

void assets_material_remove(assets *in, int id) {
    err err = CORE_SUCCESS;

    if (!in) {
        err = CORE_INVALID_NULLPTR;
        goto err;
    }

    err = array_remove(in->gl_materials, id);
    if (err)
        goto err;

    return;

err:
    logger_log(LOGGER_ERR, "Failed to remove material from assets", err);
}
