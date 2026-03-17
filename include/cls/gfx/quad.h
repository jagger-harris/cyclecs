#ifndef CLS_QUAD_H
#define CLS_QUAD_H

#include <cls/gfx/gl/mesh.h>
#include <cls/util/array.h>
#include <cls/util/util.h>

static const struct vertex QUAD_VERTICES[] = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
static const unsigned int QUAD_INDICES[] = {0, 2, 1, 0, 3, 2};
static const size_t QUAD_VERTEX_COUNT = ARRAY_LENGTH(QUAD_VERTICES);
static const size_t QUAD_INDEX_COUNT = ARRAY_LENGTH(QUAD_INDICES);

#endif // CLS_QUAD_H
