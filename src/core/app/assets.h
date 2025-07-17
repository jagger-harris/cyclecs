#ifndef APP_ASSETS_H
#define APP_ASSETS_H

#include "core/gfx/gl/mesh.h"
#include "core/gfx/material.h"
#include "core/util/arena.h"
#include "core/util/err.h"
#include "core/util/table.h"
#include <stdbool.h>

struct assets {
    struct renderer *current_renderer;
    struct table materials;
    struct table meshes;
    struct table shaders;
    struct table textures;
};

err assets_init(struct assets *out, struct arena *mem);
void assets_destroy(struct assets *in);
void assets_material_add(struct assets *in, const char *id,
                         const char *shader_path, const char *texture_path);
void assets_material_remove(struct assets *in, const char *id);
void assets_material_get(struct material **out, const struct assets *in,
                         const char *id);
void assets_mesh_add(struct assets *in, const char *id,
                     const struct vertex *vertices, size_t vertex_count,
                     unsigned int *indices, size_t index_count);
void assets_mesh_get(struct gl_mesh **out, const struct assets *in,
                     const char *id);
void assets_shader_get(unsigned int *out, const struct assets *in,
                       const char *id);
void assets_texture_get(unsigned int *out, const struct assets *in,
                        const char *id);

#endif // APP_ASSETS_H
