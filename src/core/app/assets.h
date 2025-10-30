#ifndef APP_ASSETS_H
#define APP_ASSETS_H

#include "core/gfx/texture2d.h"
#include "core/util/error.h"
#include "core/util/table.h"
#include "core/util/xxhash32.h"
#include "freetype/freetype.h"

struct ffont;
struct gfx_api;
struct gl_mesh;
struct shader;
struct vertex;

struct assets {
    struct table fonts;
    struct table materials;
    struct table meshes;
    struct table shaders;
    struct table texture2ds;
    FT_Library ft;
    struct gfx_api *api;
};

int assets_init(struct assets *out, struct gfx_api *api);
void assets_destroy(struct assets *in);
void assets_font_add(struct assets *in, const char *font_path, int pixel_size);
int assets_font_get(struct ffont **out, const struct assets *in, u32 font_id);
void assets_shader_add(struct assets *in, const char *shader_path);
int assets_shader_get(struct shader **out, const struct assets *in,
                      u32 shader_id);
void assets_texture2d_add(struct assets *in, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap);
int assets_texture2d_get(struct texture2d **out, const struct assets *in,
                         u32 texture2d_id);
void assets_mesh_add(struct assets *in, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count);
int assets_mesh_get(struct gl_mesh **out, const struct assets *in, u32 mesh_id);

#endif // APP_ASSETS_H
