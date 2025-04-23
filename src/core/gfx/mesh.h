#ifndef GFX_MESH_H
#define GFX_MESH_H

#include "core/util/array.h"
#include <cglm/cglm.h>

typedef struct mesh mesh;
struct vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct mesh {
    array *vertices;
    array *indices;
};

err mesh_new(mesh **out, array *vertices, array *indices);
err mesh_delete(mesh *in);

#endif /* GFX_MESH_H */
