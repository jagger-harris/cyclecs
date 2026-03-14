#ifndef APP_ASSETS_H
#define APP_ASSETS_H

#include <cls/gfx/texture2d.h>
#include <stddef.h>

struct allocator;
struct assets;
struct ffont;
struct gfx_api;
struct gl_mesh;
struct shader;
struct vertex;

int assets_create(struct assets **out, struct allocator *allocator,
                  struct gfx_api *api);
void assets_destroy(struct assets *in);
void assets_font_add(struct assets *in, const char *font_path, int pixel_size);
int assets_font_get(const struct ffont **out, const struct assets *in,
                    u32 font_id);
void assets_shader_add(struct assets *in, const char *shader_path);
int assets_shader_get(const struct shader **out, const struct assets *in,
                      u32 shader_id);
void assets_texture2d_add(struct assets *in, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap);
int assets_texture2d_get(const struct texture2d **out, const struct assets *in,
                         u32 texture2d_id);
void assets_mesh_add(struct assets *in, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count);
int assets_mesh_get(const struct gl_mesh **out, const struct assets *in,
                    u32 mesh_id);

#endif // APP_ASSETS_H
