/**
 * @file cls/gfx/primitive.h
 * @brief Primitive geometries for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 */

#ifndef CLS_PRIMITIVE_H
#define CLS_PRIMITIVE_H

#include <cls/gfx/gl/mesh.h>
#include <cls/util/array.h>
#include <cls/util/macros.h>

/**
 * @defgroup primitive Primitives.
 * @ingroup gfx
 * @brief Default primitive vertices and indices.
 * @{
 */

/**
 * @name Quad Geometry Data
 * Constants defining the vertices, indices, and sizes for a standard 2D quad.
 * @{
 */
static const struct cls_vertex CLS_QUAD_VERTICES[] = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
static const unsigned int CLS_QUAD_INDICES[] = {0, 2, 1, 0, 3, 2};
static const size_t CLS_QUAD_VERTEX_COUNT = CLS_ARRAY_LENGTH(CLS_QUAD_VERTICES);
static const size_t CLS_QUAD_INDEX_COUNT = CLS_ARRAY_LENGTH(CLS_QUAD_INDICES);
/** @} */

/** @} */

#endif // CLS_PRIMITIVE_H
