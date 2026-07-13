#ifndef CLS_PRIMITIVE_H
#define CLS_PRIMITIVE_H

#include <cls/gfx/gl/mesh.h>
#include <cls/util/array.h>
#include <cls/util/util.h>

static const struct cls_vertex CLS_QUAD_VERTICES[] = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
static const unsigned int CLS_QUAD_INDICES[] = {0, 2, 1, 0, 3, 2};
static const size_t CLS_QUAD_VERTEX_COUNT = CLS_ARRAY_LENGTH(CLS_QUAD_VERTICES);
static const size_t CLS_QUAD_INDEX_COUNT = CLS_ARRAY_LENGTH(CLS_QUAD_INDICES);

#endif // CLS_PRIMITIVE_H
