#include <cls/app/assets.h>
#include <cls/gfx/api.h>
#include <cls/gfx/quad.h>
#include <cls/gfx/shader.h>
#include <cls/io/ascii.h>
#include <cls/io/font.h>
#include <cls/io/image.h>
#include <cls/util/allocator.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/logger.h>
#include <cls/util/table.h>
#include <cls/util/types.h>
#include <cls/util/xxhash32.h>

#define START_CAPACITY 64
#define CORE_PATH "data/core/"
#define FONT_PATH CORE_PATH "gfx/fonts/"
#define SHADER_PATH CORE_PATH "gfx/shaders/"
#define TEXTURE2D_PATH CORE_PATH "gfx/texture2ds/"
#define TEXTURE2D_DEFAULT_BLANK "DEFAULT_BLANK"
#define TEXTURE2D_DEFAULT_MISSING "DEFAULT_MISSING"

struct assets {
    struct table *fonts;
    struct table *meshes;
    struct table *shaders;
    struct table *texture2ds;
    struct gfx_api *api;
    FT_Library ft;
};

static int load_missing_texture(struct assets *assets) {
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
    int error = assets->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = xxhash32(&id, TEXTURE2D_DEFAULT_MISSING,
                     strlen(TEXTURE2D_DEFAULT_MISSING), 0);
    if (error)
        return error;

    error = table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int load_blank_texture(struct assets *assets) {
    static u8 blank[4] = {255, 255, 255, 0};
    struct texture2d_info info = {.data = blank,
                                  .width = 1,
                                  .height = 1,
                                  .channels = 4,
                                  .filter = TEXTURE_FILTER_NEAREST,
                                  .wrap = TEXTURE_WRAP_CLAMP};
    struct texture2d texture = {0};
    int error = assets->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = xxhash32(&id, TEXTURE2D_DEFAULT_BLANK,
                     strlen(TEXTURE2D_DEFAULT_BLANK), 0);
    if (error)
        return error;

    error = table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int assets_load_defaults(struct assets *assets) {
    assets_mesh_add(assets, "quad", QUAD_VERTICES, QUAD_VERTEX_COUNT,
                    QUAD_INDICES, QUAD_INDEX_COUNT);
    assets_shader_add(assets, "font");
    assets_shader_add(assets, "mesh");
    assets_shader_add(assets, "sprite");
    assets_font_add(assets, "human_sans-regular.otf", 64);
    int error = load_missing_texture(assets);
    if (error)
        return error;

    error = load_blank_texture(assets);
    if (error)
        return error;

    return CLS_SUCCESS;
}

int assets_create(struct assets **assets, struct allocator *alloc,
                  struct gfx_api *api) {
    if (!assets || !alloc || !api)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = allocator_alloc(&instance_ptr, alloc, sizeof(struct assets),
                                alignof(struct assets));
    if (error)
        return error;

    struct assets *instance = instance_ptr;
    instance->api = api;

    error = FT_Init_FreeType(&instance->ft) ? CLS_FAILURE : CLS_SUCCESS;
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init FreeType failed");
        return error;
    }

    error = table_create(&instance->fonts, START_CAPACITY, sizeof(u32),
                         sizeof(struct font));
    if (error)
        goto cleanup;

    error = table_create(&instance->meshes, START_CAPACITY, sizeof(u32),
                         sizeof(struct gl_mesh));
    if (error)
        goto cleanup;

    error = table_create(&instance->shaders, START_CAPACITY, sizeof(u32),
                         sizeof(GLuint));
    if (error)
        goto cleanup;

    error = table_create(&instance->texture2ds, sizeof(u32), sizeof(u32),
                         sizeof(struct texture2d));
    if (error)
        goto cleanup;

    error = assets_load_defaults(instance);
    if (error)
        goto cleanup;

    *assets = instance;
    return CLS_SUCCESS;

cleanup:
    assets_destroy(instance);
    return error;
}

void assets_destroy(struct assets *assets) {
    if (!assets)
        return;

    struct table_iterator *iter = NULL;
    int error = table_iterator_create(&iter, assets->fonts);
    if (error)
        return;

    bool iter_next = false;
    while (table_iterator_next(&iter_next, iter) == CLS_SUCCESS && iter_next) {
        void *font_ptr = NULL;
        error = table_iterator_value_get(&font_ptr, iter);
        if (error)
            continue;

        struct font *font = font_ptr;
        font_destroy(font);
    }

    table_iterator_destroy(iter);

    if (assets->ft) {
        FT_Done_FreeType(assets->ft);
        assets->ft = NULL;
    }

    table_destroy(assets->fonts);
    table_destroy(assets->meshes);
    table_destroy(assets->shaders);
    table_destroy(assets->texture2ds);
}

void assets_font_add(struct assets *assets, const char *font_path,
                     int pixel_size) {
    if (!assets || !font_path)
        return;

    u32 id = 0;
    int error = xxhash32(&id, font_path, strlen(font_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = table_find(&found_ptr, assets->fonts, &id);
    if (error)
        return;

    const struct font *found = found_ptr;
    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate font in assets, not adding font (%s)", font_path);
        return;
    }

    char font_full_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(font_full_path, CLS_GLOBALS_PATH_MAX, FONT_PATH "%s",
                       font_path);
    if (ret < 0)
        return;

    struct font font = {0};
    error = font_init(&font, assets->ft, font_full_path, pixel_size);
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

    error = assets->api->texture2d_init(&texture, &info);
    if (error)
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", font_path);

    error = table_insert(assets->fonts, &id, &font);
    if (error)
        return;

    error = table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded font successfully (%s)", font_path);
    return;
}

int assets_font_get(const struct font **font, const struct assets *assets,
                    u32 font_id) {
    if (!font || !assets || !font_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = table_find(&found_ptr, assets->fonts, &font_id);
    if (error)
        return error;

    *font = found_ptr;
    return CLS_SUCCESS;
}

void assets_shader_add(struct assets *assets, const char *shader_path) {
    if (!assets || !shader_path)
        return;

    u32 id = 0;
    int error = xxhash32(&id, shader_path, strlen(shader_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = table_find(&found_ptr, assets->shaders, &id);
    if (error)
        return;

    const struct shader *found = found_ptr;
    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate shader in assets, not adding shader (%s)",
                   shader_path);
        return;
    }

    char vert_path[CLS_GLOBALS_PATH_MAX] = {0};
    char frag_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(
        vert_path, CLS_GLOBALS_PATH_MAX, "%s%.*s.vert", SHADER_PATH,
        (int)(CLS_GLOBALS_PATH_MAX - strlen(SHADER_PATH) - 6), shader_path);
    if (ret < 0)
        return;

    ret = snprintf(frag_path, CLS_GLOBALS_PATH_MAX, "%s%.*s.frag", SHADER_PATH,
                   (int)(CLS_GLOBALS_PATH_MAX - strlen(SHADER_PATH) - 6),
                   shader_path);
    if (ret < 0)
        return;

    const char *vert_src = NULL;
    const char *frag_src = NULL;

    error = ascii_init(&vert_src, vert_path);
    if (error) {
        LOGGER_LOG(LOGGER_WARN, "Reading vertex shader failed (%s)", vert_path);
        goto cleanup;
    }

    error = ascii_init(&frag_src, frag_path);
    if (error) {
        LOGGER_LOG(LOGGER_WARN, "Reading fragment shader failed (%s)",
                   frag_path);
        goto cleanup;
    }

    if (!assets->api || !assets->api->shader_init)
        goto cleanup;

    struct shader_info info = {.vert_src = vert_src, .frag_src = frag_src};
    struct shader shader = {0};
    error = assets->api->shader_init(&shader, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api shader failed (%s)", shader_path);
        goto cleanup;
    }

    ascii_destroy(&vert_src);
    ascii_destroy(&frag_src);

    error = table_insert(assets->shaders, &id, &shader);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded shader successfully (%s)", shader_path);
    return;

cleanup:
    ascii_destroy(&vert_src);
    ascii_destroy(&frag_src);
}

int assets_shader_get(struct shader **shader, const struct assets *assets,
                      u32 shader_id) {
    if (!shader || !assets || !shader_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = table_find(&found_ptr, assets->shaders, &shader_id);
    if (error)
        return error;

    *shader = found_ptr;
    return CLS_SUCCESS;
}

void assets_texture2d_add(struct assets *assets, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap) {

    if (!assets || !texture2d_path)
        return;

    if (!texture2d_path[0])
        return;

    u32 id = 0;
    int error = xxhash32(&id, texture2d_path, strlen(texture2d_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = table_find(&found_ptr, assets->texture2ds, &id);
    if (error)
        return;

    const struct texture2d *found = found_ptr;
    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate texture2d in assets, not adding texture2d (%s)",
                   texture2d_path);
        return;
    }

    char img_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(img_path, CLS_GLOBALS_PATH_MAX, "%s%.*s", TEXTURE2D_PATH,
                       (int)(CLS_GLOBALS_PATH_MAX - strlen(TEXTURE2D_PATH)),
                       texture2d_path);
    if (ret < 0)
        return;

    struct image img = {0};
    error = image_init(&img, img_path);
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
    error = assets->api->texture2d_init(&texture, &info);
    if (error) {
        LOGGER_LOG(LOGGER_ERROR, "Init api texture2d failed (%s)", img_path);
        goto cleanup;
    }

    image_destroy(&img);

    error = table_insert(assets->texture2ds, &id, &texture);
    if (error)
        goto cleanup;

    LOGGER_LOG(LOGGER_INFO, "Loaded texture successfully (%s)", img_path);
    return;

cleanup:
    image_destroy(&img);
}

int assets_texture2d_get(struct texture2d **texture,
                         const struct assets *assets, u32 texture2d_id) {
    if (!texture || !assets || !texture2d_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;

    u32 null_texture_id = 0;
    int error = xxhash32(&null_texture_id, "", strlen(""), 0);
    if (error)
        return error;

    bool has_texture = texture2d_id != null_texture_id;
    if (has_texture) {
        error = table_find(&found_ptr, assets->texture2ds, &texture2d_id);
        if (error)
            return error;

        if (!found_ptr) {
            u32 missing_id = 0;
            error = xxhash32(&missing_id, TEXTURE2D_DEFAULT_MISSING,
                             strlen(TEXTURE2D_DEFAULT_MISSING), 0);
            if (error)
                return error;

            error = table_find(&found_ptr, assets->texture2ds, &missing_id);
            if (error) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                                 "Unable to get missing texture");
                return error;
            }
        }
    } else {
        u32 blank_id = 0;
        error = xxhash32(&blank_id, TEXTURE2D_DEFAULT_BLANK,
                         strlen(TEXTURE2D_DEFAULT_BLANK), 0);
        if (error)
            return error;

        error = table_find(&found_ptr, assets->texture2ds, &blank_id);
        if (error) {
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Unable to get blank texture");
            return error;
        }
    }

    *texture = found_ptr;
    return CLS_SUCCESS;
}

void assets_mesh_add(struct assets *assets, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count) {
    if (!assets || !mesh_id || !vertices || !indices)
        return;

    u32 id = 0;
    int error = xxhash32(&id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return;

    struct gl_mesh mesh = {0};
    error = gl_mesh_init(&mesh, vertices, vertex_count, indices, index_count);
    if (error)
        return;

    void *found_ptr = NULL;
    error = table_find(&found_ptr, assets->meshes, &id);
    if (error)
        return;

    const struct mesh *found = found_ptr;
    if (found) {
        LOGGER_LOG(LOGGER_WARN,
                   "Duplicate mesh in assets, not adding mesh (%s)", mesh_id);
        return;
    }

    error = table_insert(assets->meshes, &id, &mesh);
    if (error)
        return;

    LOGGER_LOG(LOGGER_INFO, "Loaded mesh successfully (%s)", mesh_id);
    return;
}

int assets_mesh_get(struct gl_mesh **mesh, const struct assets *assets,
                    u32 mesh_id) {
    if (!mesh || !assets || !mesh_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = table_find(&found_ptr, assets->meshes, &mesh_id);
    if (error)
        return error;

    *mesh = found_ptr;
    return CLS_SUCCESS;
}
