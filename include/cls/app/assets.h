#ifndef CLS_ASSETS_H
#define CLS_ASSETS_H

#include <cls/gfx/texture2d.h>
#include <stddef.h>

struct allocator;
struct assets;
struct font;
struct gfx_api;
struct gl_mesh;
struct shader;
struct vertex;

int assets_create(struct assets **assets, struct allocator *alloc,
                  struct gfx_api *api);
void assets_destroy(struct assets *assets);
void assets_font_add(struct assets *assets, const char *font_path,
                     int pixel_size);
int assets_font_get(const struct font **f, const struct assets *assets,
                    u32 font_id);
void assets_shader_add(struct assets *assets, const char *shader_path);
int assets_shader_get(struct shader **shader, const struct assets *assets,
                      u32 shader_id);
void assets_texture2d_add(struct assets *assets, const char *texture2d_path,
                          enum texture2d_filter filter,
                          enum texture2d_wrap wrap);
int assets_texture2d_get(struct texture2d **texture,
                         const struct assets *assets, u32 texture2d_id);
void assets_mesh_add(struct assets *assets, const char *mesh_id,
                     const struct vertex *vertices, size_t vertex_count,
                     const unsigned int *indices, size_t index_count);
int assets_mesh_get(struct gl_mesh **mesh, const struct assets *assets,
                    u32 mesh_id);

#endif // CLS_ASSETS_H
