#ifndef CLS_ASSETS_H
#define CLS_ASSETS_H

#include <cls/gfx/texture2d.h>
#include <stddef.h>

struct cls_allocator;
struct cls_assets;
struct cls_font;
struct cls_gfx_api;
struct cls_gl_mesh;
struct cls_shader;
struct cls_vertex;

int cls_assets_create(struct cls_assets **assets, struct cls_allocator *alloc,
                      struct cls_gfx_api *api);
void cls_assets_destroy(struct cls_assets *assets);
void cls_assets_font_add(struct cls_assets *assets, const char *font_path,
                         int pixel_size);
int cls_assets_font_get(const struct cls_font **f,
                        const struct cls_assets *assets, u32 font_id);
void cls_assets_shader_add(struct cls_assets *assets, const char *shader_path);
int cls_assets_shader_get(struct cls_shader **shader,
                          const struct cls_assets *assets, u32 shader_id);
void cls_assets_texture2d_add(struct cls_assets *assets,
                              const char *texture2d_path,
                              enum cls_texture2d_filter filter,
                              enum cls_texture2d_wrap wrap);
int cls_assets_texture2d_get(struct cls_texture2d **texture,
                             const struct cls_assets *assets, u32 texture2d_id);
void cls_assets_mesh_add(struct cls_assets *assets, const char *mesh_id,
                         const struct cls_vertex *vertices, size_t vertex_count,
                         const unsigned int *indices, size_t index_count);
int cls_assets_mesh_get(struct cls_gl_mesh **mesh,
                        const struct cls_assets *assets, u32 mesh_id);

#endif // CLS_ASSETS_H
