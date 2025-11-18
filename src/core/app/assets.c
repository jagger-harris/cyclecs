#include "core/app/assets.h"
#include "core/gfx/api.h"
#include "core/gfx/material.h"
#include "core/gfx/quad.h"
#include "core/gfx/shader.h"
#include "core/io/fascii.h"
#include "core/io/ffont.h"
#include "core/io/fimage.h"
#include "core/util/logger.h"
#include "core/util/mem.h"
#include "core/util/table.h"
#include "core/util/types.h"
#include "core/util/xxhash32.h"

#define ASSETS_START_CAPACITY 64
#define ASSETS_DEFAULT_PATH "data/base/"
#define ASSETS_FONT_PATH ASSETS_DEFAULT_PATH "gfx/fonts/"
#define ASSETS_SHADER_PATH ASSETS_DEFAULT_PATH "gfx/shaders/"
#define ASSETS_TEXTURE2D_PATH ASSETS_DEFAULT_PATH "gfx/texture2ds/"
#define ASSETS_TEXTURE2D_DEFAULT_BLANK "DEFAULT_BLANK"
#define ASSETS_TEXTURE2D_DEFAULT_MISSING "DEFAULT_MISSING"

struct assets {
    struct table fonts;
    struct table materials;
    struct table meshes;
    struct table shaders;
    struct table texture2ds;
    FT_Library ft;
    struct gfx_api *api;
};

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
    struct texture2d texture = {0};
    int error = in->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = xxhash32(&id, ASSETS_TEXTURE2D_DEFAULT_MISSING,
                     strlen(ASSETS_TEXTURE2D_DEFAULT_MISSING), 0);
    if (error)
        return error;

    error = table_insert(&in->texture2ds, &id, &texture);
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
    struct texture2d texture = {0};
    int error = in->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = xxhash32(&id, ASSETS_TEXTURE2D_DEFAULT_BLANK,
                     strlen(ASSETS_TEXTURE2D_DEFAULT_BLANK), 0);
    if (error)
        return error;

    error = table_insert(&in->texture2ds, &id, &texture);
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
    assets_font_add(in, "human_sans-regular.otf", 64);
    int error = load_missing_texture(in);
    if (error)
        return error;

    error = load_blank_texture(in);
    if (error)
        return error;

    return CORE_SUCCESS;
}

int assets_create(struct assets **out, struct mem *mem, struct gfx_api *api) {
    if (!out || !mem || !api)
        return CORE_NULLPTR;

    struct assets *assets = NULL;
    int error = mem_alloc((void **)&assets, mem, sizeof(struct assets),
                          alignof(struct assets));
    if (error)
        return error;

    assets->api = api;

    error = FT_Init_FreeType(&assets->ft) ? CORE_FAILURE : CORE_SUCCESS;
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init FreeType failed");
        return error;
    }

    error = table_init(&assets->fonts, ASSETS_START_CAPACITY, sizeof(u32),
                       sizeof(struct ffont));
    if (error)
        goto cleanup;

    error = table_init(&assets->materials, ASSETS_START_CAPACITY, sizeof(u32),
                       sizeof(struct material));
    if (error)
        goto cleanup;

    error = table_init(&assets->meshes, ASSETS_START_CAPACITY, sizeof(u32),
                       sizeof(struct gl_mesh));
    if (error)
        goto cleanup;

    error = table_init(&assets->shaders, ASSETS_START_CAPACITY, sizeof(u32),
                       sizeof(GLuint));
    if (error)
        goto cleanup;

    error = table_init(&assets->texture2ds, sizeof(u32), sizeof(u32),
                       sizeof(struct texture2d));
    if (error)
        goto cleanup;

    error = assets_load_defaults(assets);
    if (error)
        goto cleanup;

    *out = assets;
    return CORE_SUCCESS;

cleanup:
    assets_destroy(assets);
    return error;
}

void assets_destroy(struct assets *in) {
    if (!in)
        return;

    struct table_iterator iter = {0};
    int error = table_iterator_init(&iter, &in->fonts);
    if (error)
        return;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, &iter) == CORE_SUCCESS &&
           iter_next) {
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

    u32 id = 0;
    int error = xxhash32(&id, font_path, strlen(font_path), 0);
    if (error)
        return;

    struct ffont *found = NULL;
    error = table_find((void **)&found, &in->fonts, &id);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate font in assets, not adding font (%s)", font_path);
        return;
    }

    char font_full_path[GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(font_full_path, GLOBALS_PATH_MAX, ASSETS_FONT_PATH "%s",
                       font_path);
    if (ret < 0)
        return;

    struct ffont font = {0};
    error = ffont_init(&font, in->ft, font_full_path, pixel_size);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_WARN, error,
                         "Adding font to assets failed (%s)", font_path);
        return;
    }

    struct texture2d_info info = {.data = font.atlas,
                                  .width = (int)font.atlas_width,
                                  .height = (int)font.atlas_height,
                                  .channels = 1,
                                  .filter = TEXTURE_FILTER_LINEAR,
                                  .wrap = TEXTURE_WRAP_CLAMP};
    struct texture2d texture = {0};

    error = in->api->texture2d_init(&texture, &info);
    if (error)
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", font_path);

    error = table_insert(&in->fonts, &id, &font);
    if (error)
        return;

    error = table_insert(&in->texture2ds, &id, &texture);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded font successfully (%s)", font_path);
    return;
}

int assets_font_get(struct ffont **out, const struct assets *in, u32 font_id) {
    if (!out || !in || !font_id)
        return CORE_NULLPTR;

    int error = table_find((void **)out, &in->fonts, &font_id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

void assets_shader_add(struct assets *in, const char *shader_path) {
    if (!in || !shader_path)
        return;

    u32 id = 0;
    int error = xxhash32(&id, shader_path, strlen(shader_path), 0);
    if (error)
        return;

    struct shader *found = NULL;
    error = table_find((void **)&found, &in->shaders, &id);
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
    int ret = snprintf(
        vert_path, GLOBALS_PATH_MAX, "%s%.*s.vert", ASSETS_SHADER_PATH,
        (int)(GLOBALS_PATH_MAX - strlen(ASSETS_SHADER_PATH) - 6), shader_path);
    if (ret < 0)
        return;

    ret = snprintf(
        frag_path, GLOBALS_PATH_MAX, "%s%.*s.frag", ASSETS_SHADER_PATH,
        (int)(GLOBALS_PATH_MAX - strlen(ASSETS_SHADER_PATH) - 6), shader_path);
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
    struct shader shader = {0};
    error = in->api->shader_init(&shader, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api shader failed (%s)", shader_path);
        goto cleanup;
    }

    fascii_destroy(&vert_src);
    fascii_destroy(&frag_src);

    error = table_insert(&in->shaders, &id, &shader);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded shader successfully (%s)", shader_path);
    return;

cleanup:
    fascii_destroy(&vert_src);
    fascii_destroy(&frag_src);
}

int assets_shader_get(struct shader **out, const struct assets *in,
                      u32 shader_id) {
    if (!in || !shader_id)
        return CORE_NULLPTR;

    int error = table_find((void **)out, &in->shaders, &shader_id);
    if (error)
        return error;

    return CORE_SUCCESS;
}

void assets_texture2d_add(struct assets *in, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap) {

    if (!in || !texture2d_path)
        return;

    if (!texture2d_path[0])
        return;

    u32 id = 0;
    int error = xxhash32(&id, texture2d_path, strlen(texture2d_path), 0);
    if (error)
        return;

    struct texture2d *found = NULL;
    error = table_find((void **)&found, &in->texture2ds, &id);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate texture2d in assets, not adding texture2d (%s)",
                   texture2d_path);
        return;
    }

    char img_path[GLOBALS_PATH_MAX] = {0};
    int ret =
        snprintf(img_path, GLOBALS_PATH_MAX, "%s%.*s", ASSETS_TEXTURE2D_PATH,
                 (int)(GLOBALS_PATH_MAX - strlen(ASSETS_TEXTURE2D_PATH)),
                 texture2d_path);
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
    struct texture2d texture = {0};
    error = in->api->texture2d_init(&texture, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", img_path);
        goto cleanup;
    }

    fimage_destroy(&img);

    error = table_insert(&in->texture2ds, &id, &texture);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded texture successfully (%s)", img_path);
    return;

cleanup:
    fimage_destroy(&img);
}

int assets_texture2d_get(struct texture2d **out, const struct assets *in,
                         u32 texture2d_id) {
    if (!in || !texture2d_id)
        return CORE_NULLPTR;

    struct texture2d *found = NULL;

    u32 null_texture_id = 0;
    int error = xxhash32(&null_texture_id, "", strlen(""), 0);
    if (error)
        return error;

    bool has_texture = texture2d_id != null_texture_id;
    if (has_texture) {
        error = table_find((void **)&found, &in->texture2ds, &texture2d_id);
        if (error)
            return error;

        if (!found) {
            u32 missing_id = 0;
            error = xxhash32(&missing_id, ASSETS_TEXTURE2D_DEFAULT_MISSING,
                             strlen(ASSETS_TEXTURE2D_DEFAULT_MISSING), 0);
            if (error)
                return error;

            error = table_find((void **)&found, &in->texture2ds, &missing_id);
            if (error) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                                 "Unable to get missing texture");
                return error;
            }
        }
    } else {
        u32 blank_id = 0;
        error = xxhash32(&blank_id, ASSETS_TEXTURE2D_DEFAULT_BLANK,
                         strlen(ASSETS_TEXTURE2D_DEFAULT_BLANK), 0);
        if (error)
            return error;

        error = table_find((void **)&found, &in->texture2ds, &blank_id);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Unable to get blank texture");
            return error;
        }
    }

    *out = found;
    return CORE_SUCCESS;
}

void assets_mesh_add(struct assets *in, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count) {
    if (!in || !mesh_id || !vertices || !indices)
        return;

    u32 id = 0;
    int error = xxhash32(&id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return;

    struct gl_mesh mesh = {0};
    error = gl_mesh_init(&mesh, vertices, vertex_count, indices, index_count);
    if (error)
        return;

    struct mesh *found = NULL;
    error = table_find((void **)&found, &in->meshes, &id);
    if (error)
        return;

    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate mesh in assets, not adding mesh (%s)", mesh_id);
        return;
    }

    error = table_insert(&in->meshes, &id, &mesh);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded mesh successfully (%s)", mesh_id);
    return;
}

int assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                    u32 mesh_id) {
    if (!out || !in || !mesh_id)
        return CORE_NULLPTR;

    int error = table_find((void **)out, &in->meshes, &mesh_id);
    if (error)
        return error;

    return CORE_SUCCESS;
}
