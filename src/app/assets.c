#include <cls/app/assets.h>
#include <cls/gfx/gfx_api.h>
#include <cls/gfx/quad.h>
#include <cls/gfx/shader.h>
#include <cls/io/ascii.h>
#include <cls/io/font.h>
#include <cls/io/image.h>
#include <cls/util/error.h>
#include <cls/util/globals.h>
#include <cls/util/logger.h>
#include <cls/util/mem.h>
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

struct cls_assets {
    struct cls_table *fonts;
    struct cls_table *meshes;
    struct cls_table *shaders;
    struct cls_table *texture2ds;
    struct cls_gfx_api *api;
    FT_Library ft;
};

static int load_missing_texture(struct cls_assets *assets) {
    static u8 missingno[16] = {
        255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255,
    };
    struct cls_texture2d_info info = {.data = missingno,
                                      .width = 2,
                                      .height = 2,
                                      .channels = 4,
                                      .filter = CLS_TEXTURE_FILTER_NEAREST,
                                      .wrap = CLS_TEXTURE_WRAP_REPEAT};
    struct cls_texture2d texture = {0};
    int error = assets->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = cls_xxhash32(&id, TEXTURE2D_DEFAULT_MISSING,
                         strlen(TEXTURE2D_DEFAULT_MISSING), 0);
    if (error)
        return error;

    error = cls_table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int load_blank_texture(struct cls_assets *assets) {
    static u8 blank[4] = {255, 255, 255, 0};
    struct cls_texture2d_info info = {.data = blank,
                                      .width = 1,
                                      .height = 1,
                                      .channels = 4,
                                      .filter = CLS_TEXTURE_FILTER_NEAREST,
                                      .wrap = CLS_TEXTURE_WRAP_CLAMP};
    struct cls_texture2d texture = {0};
    int error = assets->api->texture2d_init(&texture, &info);
    if (error)
        return error;

    u32 id = 0;
    error = cls_xxhash32(&id, TEXTURE2D_DEFAULT_BLANK,
                         strlen(TEXTURE2D_DEFAULT_BLANK), 0);
    if (error)
        return error;

    error = cls_table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return error;

    return CLS_SUCCESS;
}

static int assets_load_defaults(struct cls_assets *assets) {
    cls_assets_mesh_add(assets, "quad", CLS_QUAD_VERTICES,
                        CLS_QUAD_VERTEX_COUNT, CLS_QUAD_INDICES,
                        CLS_QUAD_INDEX_COUNT);
    cls_assets_shader_add(assets, "font");
    cls_assets_shader_add(assets, "mesh");
    cls_assets_shader_add(assets, "sprite");
    cls_assets_font_add(assets, "human_sans-regular.otf", 64);
    int error = load_missing_texture(assets);
    if (error)
        return error;

    error = load_blank_texture(assets);
    if (error)
        return error;

    return CLS_SUCCESS;
}

int cls_assets_create(struct cls_assets **assets, struct cls_mem *mem,
                      struct cls_gfx_api *api) {
    if (!assets || !mem || !api)
        return CLS_NULLPTR;

    void *instance_ptr = NULL;
    int error = cls_mem_alloc(&instance_ptr, mem, sizeof(struct cls_assets),
                              alignof(struct cls_assets));
    if (error)
        return error;

    struct cls_assets *instance = instance_ptr;
    instance->api = api;

    error = FT_Init_FreeType(&instance->ft) ? CLS_FAILURE : CLS_SUCCESS;
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Init FreeType failed");
        return error;
    }

    error = cls_table_create(&instance->fonts, START_CAPACITY, sizeof(u32),
                             sizeof(struct cls_font));
    if (error)
        goto cleanup;

    error = cls_table_create(&instance->meshes, START_CAPACITY, sizeof(u32),
                             sizeof(struct cls_gl_mesh));
    if (error)
        goto cleanup;

    error = cls_table_create(&instance->shaders, START_CAPACITY, sizeof(u32),
                             sizeof(GLuint));
    if (error)
        goto cleanup;

    error = cls_table_create(&instance->texture2ds, sizeof(u32), sizeof(u32),
                             sizeof(struct cls_texture2d));
    if (error)
        goto cleanup;

    error = assets_load_defaults(instance);
    if (error)
        goto cleanup;

    *assets = instance;
    return CLS_SUCCESS;

cleanup:
    cls_assets_destroy(instance);
    return error;
}

void cls_assets_destroy(struct cls_assets *assets) {
    if (!assets)
        return;

    struct cls_table_iterator *iter = NULL;
    int error = cls_table_iterator_create(&iter, assets->fonts);
    if (error)
        return;

    bool iter_next = false;
    while (cls_table_iterator_next(&iter_next, iter) == CLS_SUCCESS &&
           iter_next) {
        void *font_ptr = NULL;
        error = cls_table_iterator_value_get(&font_ptr, iter);
        if (error)
            continue;

        struct cls_font *font = font_ptr;
        cls_font_destroy(font);
    }

    cls_table_iterator_destroy(iter);

    if (assets->ft) {
        FT_Done_FreeType(assets->ft);
        assets->ft = NULL;
    }

    cls_table_destroy(assets->fonts);
    cls_table_destroy(assets->meshes);
    cls_table_destroy(assets->shaders);
    cls_table_destroy(assets->texture2ds);
}

void cls_assets_font_add(struct cls_assets *assets, const char *font_path,
                         int pixel_size) {
    if (!assets || !font_path)
        return;

    u32 id = 0;
    int error = cls_xxhash32(&id, font_path, strlen(font_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = cls_table_find(&found_ptr, assets->fonts, &id);
    if (error)
        return;

    const struct font *found = found_ptr;
    if (found) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN,
                       "Duplicate font in assets, not adding font (%s)",
                       font_path);
        return;
    }

    char font_full_path[CLS_GLOBALS_PATH_MAX] = {0};
    int ret = snprintf(font_full_path, CLS_GLOBALS_PATH_MAX, FONT_PATH "%s",
                       font_path);
    if (ret < 0)
        return;

    struct cls_font font = {0};
    error = cls_font_init(&font, assets->ft, font_full_path, pixel_size);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_WARN, error,
                             "Adding font to assets failed (%s)", font_path);
        return;
    }

    struct cls_texture2d_info info = {.data = font.atlas,
                                      .width = (int)font.atlas_width,
                                      .height = (int)font.atlas_height,
                                      .channels = 1,
                                      .filter = CLS_TEXTURE_FILTER_LINEAR,
                                      .wrap = CLS_TEXTURE_WRAP_CLAMP};
    struct cls_texture2d texture = {0};

    error = assets->api->texture2d_init(&texture, &info);
    if (error)
        CLS_LOGGER_LOG(CLS_LOGGER_ERROR, "Init api texture2d failed (%s)",
                       font_path);

    error = cls_table_insert(assets->fonts, &id, &font);
    if (error)
        return;

    error = cls_table_insert(assets->texture2ds, &id, &texture);
    if (error)
        return;

    CLS_LOGGER_LOG(CLS_LOGGER_INFO, "Loaded font successfully (%s)", font_path);
    return;
}

int cls_assets_font_get(const struct cls_font **font,
                        const struct cls_assets *assets, u32 font_id) {
    if (!font || !assets || !font_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = cls_table_find(&found_ptr, assets->fonts, &font_id);
    if (error)
        return error;

    *font = found_ptr;
    return CLS_SUCCESS;
}

void cls_assets_shader_add(struct cls_assets *assets, const char *shader_path) {
    if (!assets || !shader_path)
        return;

    u32 id = 0;
    int error = cls_xxhash32(&id, shader_path, strlen(shader_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = cls_table_find(&found_ptr, assets->shaders, &id);
    if (error)
        return;

    const struct shader *found = found_ptr;
    if (found) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN,
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

    error = cls_ascii_init(&vert_src, vert_path);
    if (error) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN, "Reading vertex shader failed (%s)",
                       vert_path);
        goto cleanup;
    }

    error = cls_ascii_init(&frag_src, frag_path);
    if (error) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN, "Reading fragment shader failed (%s)",
                       frag_path);
        goto cleanup;
    }

    if (!assets->api || !assets->api->shader_init)
        goto cleanup;

    struct cls_shader_info info = {.vert_src = vert_src, .frag_src = frag_src};
    struct cls_shader shader = {0};
    error = assets->api->shader_init(&shader, &info);
    if (error) {
        CLS_LOGGER_LOG(CLS_LOGGER_ERROR, "Init api shader failed (%s)",
                       shader_path);
        goto cleanup;
    }

    cls_ascii_destroy(&vert_src);
    cls_ascii_destroy(&frag_src);

    error = cls_table_insert(assets->shaders, &id, &shader);
    if (error)
        goto cleanup;

    CLS_LOGGER_LOG(CLS_LOGGER_INFO, "Loaded shader successfully (%s)",
                   shader_path);
    return;

cleanup:
    cls_ascii_destroy(&vert_src);
    cls_ascii_destroy(&frag_src);
}

int cls_assets_shader_get(struct cls_shader **shader,
                          const struct cls_assets *assets, u32 shader_id) {
    if (!shader || !assets || !shader_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = cls_table_find(&found_ptr, assets->shaders, &shader_id);
    if (error)
        return error;

    *shader = found_ptr;
    return CLS_SUCCESS;
}

void cls_assets_texture2d_add(struct cls_assets *assets,
                              const char *texture2d_path,
                              enum cls_texture2d_filter filter,
                              enum cls_texture2d_wrap wrap) {

    if (!assets || !texture2d_path)
        return;

    if (!texture2d_path[0])
        return;

    u32 id = 0;
    int error = cls_xxhash32(&id, texture2d_path, strlen(texture2d_path), 0);
    if (error)
        return;

    void *found_ptr = NULL;
    error = cls_table_find(&found_ptr, assets->texture2ds, &id);
    if (error)
        return;

    const struct texture2d *found = found_ptr;
    if (found) {
        CLS_LOGGER_LOG(
            CLS_LOGGER_WARN,
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

    struct cls_image img = {0};
    error = cls_image_init(&img, img_path);
    if (error) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN, "Reading image file failed (%s)",
                       img_path);
        goto cleanup;
    }

    struct cls_texture2d_info info = {.data = img.data,
                                      .width = img.width,
                                      .height = img.height,
                                      .channels = img.channels,
                                      .filter = filter,
                                      .wrap = wrap};
    struct cls_texture2d texture = {0};
    error = assets->api->texture2d_init(&texture, &info);
    if (error) {
        CLS_LOGGER_LOG(CLS_LOGGER_ERROR, "Init api texture2d failed (%s)",
                       img_path);
        goto cleanup;
    }

    cls_image_destroy(&img);

    error = cls_table_insert(assets->texture2ds, &id, &texture);
    if (error)
        goto cleanup;

    CLS_LOGGER_LOG(CLS_LOGGER_INFO, "Loaded texture successfully (%s)",
                   img_path);
    return;

cleanup:
    cls_image_destroy(&img);
}

int cls_assets_texture2d_get(struct cls_texture2d **texture,
                             const struct cls_assets *assets,
                             u32 texture2d_id) {
    if (!texture || !assets || !texture2d_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;

    u32 null_texture_id = 0;
    int error = cls_xxhash32(&null_texture_id, "", strlen(""), 0);
    if (error)
        return error;

    bool has_texture = texture2d_id != null_texture_id;
    if (has_texture) {
        error = cls_table_find(&found_ptr, assets->texture2ds, &texture2d_id);
        if (error)
            return error;

        if (!found_ptr) {
            u32 missing_id = 0;
            error = cls_xxhash32(&missing_id, TEXTURE2D_DEFAULT_MISSING,
                                 strlen(TEXTURE2D_DEFAULT_MISSING), 0);
            if (error)
                return error;

            error = cls_table_find(&found_ptr, assets->texture2ds, &missing_id);
            if (error) {
                CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                     "Unable to get missing texture");
                return error;
            }
        }
    } else {
        u32 blank_id = 0;
        error = cls_xxhash32(&blank_id, TEXTURE2D_DEFAULT_BLANK,
                             strlen(TEXTURE2D_DEFAULT_BLANK), 0);
        if (error)
            return error;

        error = cls_table_find(&found_ptr, assets->texture2ds, &blank_id);
        if (error) {
            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                 "Unable to get blank texture");
            return error;
        }
    }

    *texture = found_ptr;
    return CLS_SUCCESS;
}

void cls_assets_mesh_add(struct cls_assets *assets, const char *mesh_id,
                         const struct cls_vertex *vertices, size_t vertex_count,
                         const unsigned int *indices, size_t index_count) {
    if (!assets || !mesh_id || !vertices || !indices)
        return;

    u32 id = 0;
    int error = cls_xxhash32(&id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return;

    struct cls_gl_mesh mesh = {0};
    error =
        cls_gl_mesh_init(&mesh, vertices, vertex_count, indices, index_count);
    if (error)
        return;

    void *found_ptr = NULL;
    error = cls_table_find(&found_ptr, assets->meshes, &id);
    if (error)
        return;

    const struct mesh *found = found_ptr;
    if (found) {
        CLS_LOGGER_LOG(CLS_LOGGER_WARN,
                       "Duplicate mesh in assets, not adding mesh (%s)",
                       mesh_id);
        return;
    }

    error = cls_table_insert(assets->meshes, &id, &mesh);
    if (error)
        return;

    CLS_LOGGER_LOG(CLS_LOGGER_INFO, "Loaded mesh successfully (%s)", mesh_id);
    return;
}

int cls_assets_mesh_get(struct cls_gl_mesh **mesh,
                        const struct cls_assets *assets, u32 mesh_id) {
    if (!mesh || !assets || !mesh_id)
        return CLS_NULLPTR;

    void *found_ptr = NULL;
    int error = cls_table_find(&found_ptr, assets->meshes, &mesh_id);
    if (error)
        return error;

    *mesh = found_ptr;
    return CLS_SUCCESS;
}
