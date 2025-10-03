#include "core/app/assets.h"
#include "core/gfx/api.h"
#include "core/gfx/material.h"
#include "core/gfx/quad.h"
#include "core/gfx/shader.h"
#include "core/io/fascii.h"
#include "core/io/ffont.h"
#include "core/io/fimage.h"
#include "core/util/globals.h"
#include "core/util/logger.h"
#include "core/util/types.h"

#define ASSETS_START_CAPACITY 64
#define ASSETS_DEFAULT_PATH "data/base/"
#define ASSETS_FONT_PATH ASSETS_DEFAULT_PATH "gfx/fonts/"
#define ASSETS_SHADER_PATH ASSETS_DEFAULT_PATH "gfx/shaders/"
#define ASSETS_TEXTURE2D_PATH ASSETS_DEFAULT_PATH "gfx/texture2ds/"

static int load_missing_texture(struct assets *in) {
    static u8 missingno[16] = {
        255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255,
    };
    struct texture2d_info info = {.data = missingno,
                                  .width = 2,
                                  .height = 2,
                                  .channels = 4,
                                  .filter = TEXTURE_FILTER_NEAREST,
                                  .wrap = TEXTURE_WRAP_REPEAT};
    struct texture2d new_texture = {0};
    int error = in->api->texture2d_init(&new_texture, &info);
    if (error)
        return error;

    char path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(path, GLOBALS_PATH_MAX, "%s", "DEFAULT_MISSING");
    if (ret < 0)
        return CORE_FAILURE;

    error = table_insert(&in->texture2ds, path, &new_texture);
    if (error)
        return error;

    return CORE_SUCCESS;
}

static int load_blank_texture(struct assets *in) {
    static u8 blank[4] = {255, 255, 255, 0};
    struct texture2d_info info = {.data = blank,
                                  .width = 1,
                                  .height = 1,
                                  .channels = 4,
                                  .filter = TEXTURE_FILTER_NEAREST,
                                  .wrap = TEXTURE_WRAP_CLAMP};
    struct texture2d new_texture = {0};
    int error = in->api->texture2d_init(&new_texture, &info);
    if (error)
        return error;

    char path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(path, GLOBALS_PATH_MAX, "%s", "DEFAULT_BLANK");
    if (ret < 0)
        return CORE_FAILURE;

    error = table_insert(&in->texture2ds, path, &new_texture);
    if (error)
        return error;

    return CORE_SUCCESS;
}

static int assets_load_defaults(struct assets *in) {
    assets_mesh_add(in, "quad", QUAD_VERTICES, QUAD_VERTEX_COUNT, QUAD_INDICES,
                    QUAD_INDEX_COUNT);
    assets_shader_add(in, "font");
    assets_shader_add(in, "mesh");
    assets_shader_add(in, "sprite");
    int error = load_missing_texture(in);
    if (error)
        return error;

    error = load_blank_texture(in);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int assets_init(struct assets *out, struct gfx_api *api) {
    if (!out)
        return CORE_NULLPTR;

    *out = (struct assets){.api = api};

    int error = FT_Init_FreeType(&out->ft) ? CORE_FAILURE : CORE_SUCCESS;
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init FreeType failed");
        return error;
    }

    error = table_init(&out->fonts, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                       sizeof(struct ffont));
    if (error)
        goto cleanup;

    error = table_init(&out->materials, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                       sizeof(struct material));
    if (error)
        goto cleanup;

    error = table_init(&out->meshes, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                       sizeof(struct gl_mesh));
    if (error)
        goto cleanup;

    error = table_init(&out->shaders, ASSETS_START_CAPACITY, GLOBALS_PATH_MAX,
                       sizeof(GLuint));
    if (error)
        goto cleanup;

    error = table_init(&out->texture2ds, ASSETS_START_CAPACITY,
                       GLOBALS_PATH_MAX, sizeof(struct texture2d));
    if (error)
        goto cleanup;

    error = assets_load_defaults(out);
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    assets_destroy(out);
    return error;
}

void assets_destroy(struct assets *in) {
    if (!in)
        return;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->fonts);
    if (error)
        return;

    while (table_iterator_next(&iter)) {
        struct ffont *font = iter.value;
        ffont_destroy(font);
    }

    if (in->ft) {
        FT_Done_FreeType(in->ft);
        in->ft = NULL;
    }

    table_destroy(&in->fonts);
    table_destroy(&in->materials);
    table_destroy(&in->meshes);
    table_destroy(&in->shaders);
    table_destroy(&in->texture2ds);
}

void assets_font_add(struct assets *in, const char *font_path, int pixel_size) {
    if (!in || !font_path)
        return;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", font_path);
    if (ret < 0)
        return;

    struct ffont *found = NULL;
    int error = table_find((void **)&found, &in->fonts, id);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate font in assets, not adding font (%s)", font_path);
        return;
    }

    char path[GLOBALS_PATH_MAX] = {0};
    ret = snprintf(path, GLOBALS_PATH_MAX, ASSETS_FONT_PATH "%s", font_path);
    if (ret < 0)
        return;

    struct ffont new_font = {0};
    error = ffont_init(&new_font, in->ft, path, pixel_size);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_WARN, error,
                         "Adding font to assets failed (%s)", path);
        return;
    }

    struct texture2d_info info = {.data = new_font.atlas,
                                  .width = (int)new_font.atlas_width,
                                  .height = (int)new_font.atlas_height,
                                  .channels = 1,
                                  .filter = TEXTURE_FILTER_LINEAR,
                                  .wrap = TEXTURE_WRAP_CLAMP};
    struct texture2d new_texture = {0};

    error = in->api->texture2d_init(&new_texture, &info);
    if (error)
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", font_path);

    error = table_insert(&in->fonts, id, &new_font);
    if (error)
        return;

    error = table_insert(&in->texture2ds, id, &new_texture);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded font successfully (%s)", path);
    return;
}

int assets_font_get(struct ffont **out, const struct assets *in,
                    const char *font_id) {
    if (!out || !in || !font_id)
        return CORE_NULLPTR;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", font_id);
    if (ret < 0)
        return CORE_FAILURE;

    int error = table_find((void **)out, &in->fonts, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

void assets_shader_add(struct assets *in, const char *shader_path) {
    if (!in || !shader_path)
        return;

    char path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(path, GLOBALS_PATH_MAX, "%s", shader_path);
    if (ret < 0)
        return;

    struct shader *found = NULL;
    int error = table_find((void **)&found, &in->shaders, path);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate shader in assets, not adding shader (%s)",
                   shader_path);
        return;
    }

    char vert_path[GLOBALS_PATH_MAX] = {0};
    char frag_path[GLOBALS_PATH_MAX] = {0};
    ret = snprintf(
        vert_path, GLOBALS_PATH_MAX, "%s%.*s.vert", ASSETS_SHADER_PATH,
        (int)(GLOBALS_PATH_MAX - strlen(ASSETS_SHADER_PATH) - 6), path);
    if (ret < 0)
        return;

    ret = snprintf(
        frag_path, GLOBALS_PATH_MAX, "%s%.*s.frag", ASSETS_SHADER_PATH,
        (int)(GLOBALS_PATH_MAX - strlen(ASSETS_SHADER_PATH) - 6), path);
    if (ret < 0)
        return;

    const char *vert_src = NULL;
    const char *frag_src = NULL;

    error = fascii_init(&vert_src, vert_path);
    if (error) {
        LOGGER_LOG(LOGGER_WARN, "Reading vertex shader failed (%s)", vert_path);
        goto cleanup;
    }

    error = fascii_init(&frag_src, frag_path);
    if (error) {
        LOGGER_LOG(LOGGER_WARN, "Reading fragment shader failed (%s)",
                   frag_path);
        goto cleanup;
    }

    if (!in->api || !in->api->shader_init)
        goto cleanup;

    struct shader_info info = {.vert_src = vert_src, .frag_src = frag_src};
    struct shader new_shader = {0};
    error = in->api->shader_init(&new_shader, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api shader failed (%s)", path);
        goto cleanup;
    }

    fascii_destroy(&vert_src);
    fascii_destroy(&frag_src);

    error = table_insert(&in->shaders, path, &new_shader);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded shader successfully (%s)", path);
    return;

cleanup:
    fascii_destroy(&vert_src);
    fascii_destroy(&frag_src);
}

int assets_shader_get(struct shader **out, const struct assets *in,
                      const char *shader_id) {
    if (!in || !shader_id)
        return CORE_NULLPTR;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", shader_id);
    if (ret < 0)
        return CORE_FAILURE;

    int error = table_find((void **)out, &in->shaders, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

void assets_texture2d_add(struct assets *in, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap) {

    if (!in || !texture2d_path)
        return;

    char path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(path, GLOBALS_PATH_MAX, "%s", texture2d_path);
    if (ret < 0)
        return;

    struct texture2d *found = NULL;
    int error = table_find((void **)&found, &in->texture2ds, path);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate texture2d in assets, not adding texture2d (%s)",
                   path);
        return;
    }

    char img_path[GLOBALS_PATH_MAX] = {0};
    ret = snprintf(img_path, GLOBALS_PATH_MAX, "%s%.*s", ASSETS_TEXTURE2D_PATH,
                   (int)(GLOBALS_PATH_MAX - strlen(ASSETS_TEXTURE2D_PATH) - 1),
                   path);
    if (ret < 0)
        return;

    struct fimage img = {0};
    error = fimage_init(&img, img_path);
    if (error) {
        LOGGER_LOG(LOGGER_WARN, "Reading image file failed (%s)", img_path);
        goto cleanup;
    }

    struct texture2d_info info = {.data = img.data,
                                  .width = img.width,
                                  .height = img.height,
                                  .channels = img.channels,
                                  .filter = filter,
                                  .wrap = wrap};
    struct texture2d new_texture = {0};
    error = in->api->texture2d_init(&new_texture, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", img_path);
        goto cleanup;
    }

    fimage_destroy(&img);

    error = table_insert(&in->texture2ds, path, &new_texture);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded texture successfully (%s)", img_path);
    return;

cleanup:
    fimage_destroy(&img);
}

int assets_texture2d_get(struct texture2d **out, const struct assets *in,
                         const char *texture_id) {
    if (!in || !texture_id)
        return CORE_NULLPTR;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", texture_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct texture2d *found = NULL;
    bool has_texture = texture_id[0] ? true : false;

    if (has_texture) {
        int error = table_find((void **)&found, &in->texture2ds, id);
        if (error)
            return error;

        if (!found) {
            char missing_id[GLOBALS_PATH_MAX] = {0};
            int ret =
                snprintf(missing_id, GLOBALS_PATH_MAX, "%s", "DEFAULT_MISSING");
            if (ret < 0)
                return CORE_FAILURE;

            error = table_find((void **)&found, &in->texture2ds, missing_id);
            if (error) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                                 "Unable to load missing texture");
                return error;
            }
        }
    } else {
        char blank_id[GLOBALS_PATH_MAX] = {0};
        int ret = snprintf(blank_id, GLOBALS_PATH_MAX, "%s", "DEFAULT_BLANK");
        if (ret < 0)
            return CORE_FAILURE;

        int error = table_find((void **)&found, &in->texture2ds, blank_id);
        if (error)
            return error;
    }

    *out = found;
    return CORE_SUCCESS;
}

void assets_mesh_add(struct assets *in, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count) {
    if (!in || !mesh_id || !vertices || !indices)
        return;

    struct gl_mesh new_mesh = {0};
    int error =
        gl_mesh_init(&new_mesh, vertices, vertex_count, indices, index_count);
    if (error)
        return;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", mesh_id);
    if (ret < 0)
        return;

    struct mesh *found = NULL;
    error = table_find((void **)&found, &in->meshes, id);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate mesh in assets, not adding mesh (%s)", mesh_id);
        return;
    }

    error = table_insert(&in->meshes, id, &new_mesh);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded mesh successfully (%s)", mesh_id);
    return;
}

int assets_mesh_remove(struct assets *in, const char *mesh_id) {
    if (!in || !mesh_id)
        return CORE_NULLPTR;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", mesh_id);
    if (ret)
        return CORE_FAILURE;

    int error = table_remove(NULL, &in->meshes, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                    const char *mesh_id) {
    if (!out || !in || !mesh_id)
        return CORE_NULLPTR;

    char id[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(id, GLOBALS_PATH_MAX, "%s", mesh_id);
    if (ret < 0)
        return CORE_FAILURE;

    int error = table_find((void **)out, &in->meshes, id);
    if (error)
        return error;

    return CORE_SUCCESS;
}
