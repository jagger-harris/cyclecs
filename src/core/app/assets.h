#ifndef APP_ASSETS_H
#define APP_ASSETS_H

#include "core/gfx/gl/mesh.h"
#include "core/gfx/material.h"
#include "core/util/table.h"
#include <stdbool.h>

struct assets {
    struct renderer *current_renderer;
    struct table materials;
    struct table meshes;
    struct table shaders;
    struct table textures;
};

int assets_init(struct assets *out);
void assets_destroy(struct assets *in);
int assets_material_add(struct assets *in, const char *id,
                        const char *shader_path, const char *texture_path);
int assets_material_remove(struct assets *in, const char *id);
int assets_material_get(struct material **out, const struct assets *in,
                        const char *id);
int assets_mesh_add(struct assets *in, const char *id,
                    const struct vertex *vertices, size_t vertex_count,
                    unsigned int *indices, size_t index_count);
int assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                    const char *id);
int assets_shader_get(unsigned int *out, const struct assets *in,
                      const char *id);
int assets_texture_get(unsigned int *out, const struct assets *in,
                       const char *id);

#endif // APP_ASSETS_H
