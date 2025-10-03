#ifndef GFX_QUAD_H
#define GFX_QUAD_H

#include "core/gfx/gl/mesh.h"
#include "core/util/array.h"
#include "core/util/util.h"

static const struct vertex QUAD_VERTICES[] = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
static const unsigned int QUAD_INDICES[] = {0, 2, 1, 0, 3, 2};
static const size_t QUAD_VERTEX_COUNT = ARRAY_LENGTH(QUAD_VERTICES);
static const size_t QUAD_INDEX_COUNT = ARRAY_LENGTH(QUAD_INDICES);

#endif // GFX_QUAD_H
