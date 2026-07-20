/**
 * @file cls/app/assets.h
 * @brief Assets management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/app/assets.c
 */

#ifndef CLS_ASSETS_H
#define CLS_ASSETS_H

#include <cls/gfx/texture2d.h>
#include <cls/util/error.h>
#include <stddef.h>

/* Forward declarations. */
struct cls_font;
struct cls_gfx_api;
struct cls_gl_mesh;
struct cls_mem;
struct cls_shader;
struct cls_vertex;
struct cls_renderer_api;

/**
 * @defgroup assets Assets
 * @ingroup app
 * @brief Assets state and logic for the application.
 * @{
 */

/**
 * @brief Sentinel value representing an invalid font.
 */
static const u32 CLS_FONT_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid OpenGL mesh.
 */
static const u32 CLS_GL_MESH_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid shader.
 */
static const u32 CLS_SHADER_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid texture2d.
 */
static const u32 CLS_TEXTURE2D_MAX = U32_MAX;

/**
 * @brief Font identifier.
 *
 * A value of CLS_FONT_MAX represents an invalid font.
 */
typedef u32 cls_font_id;

/**
 * @brief OpenGL mesh identifier.
 *
 * A value of CLS_GL_MESH_MAX represents an invalid OpenGl mesh.
 */
typedef u32 cls_gl_mesh_id;

/**
 * @brief Shader identifier.
 *
 * A value of CLS_SHADER_MAX represents an invalid shader.
 */
typedef u32 cls_shader_id;

/**
 * @brief Texture2D identifier.
 *
 * A value of CLS_TEXTURE2D_MAX represents an invalid texture2d.
 */
typedef u32 cls_texture2d_id;

/**
 * @enum
 * @brief Default meshes.
 */
enum { CLS_MESH_QUAD = 0 };

/**
 * @enum
 * @brief Default shaders.
 */
enum { CLS_SHADER_FONT = 0, CLS_SHADER_MESH, CLS_SHADER_SPRITE };

/**
 * @enum
 * @brief Default fonts.
 */
enum { CLS_FONT_HUMANSANS_REG = 0 };

static const cls_texture2d_id CLS_TEXTURE2D_MISSING = CLS_TEXTURE2D_MAX;
static const cls_texture2d_id CLS_TEXTURE2D_BLANK = CLS_TEXTURE2D_MAX - 1;

/**
 * @struct cls_assets
 * @brief Asset store.
 */
struct cls_assets;

/**
 * @brief Creates an asset store.
 *
 * Allocates an asset store, initializes FreeType, creates the internal asset
 * tables, and loads the default assets. Destroy the returned store with
 * cls_assets_destroy().
 *
 * @param[out] assets Asset store.
 * @param[in]  mem    Memory allocator used to allocate the store.
 * @param[in]  api    Graphics API used to initialize GPU resources.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `assets`, `mem`, or `api` is NULL.
 * @retval CLS_FAILURE If FreeType initialization fails.
 * @retval (error)     If creating the asset tables or loading the default
 *                     assets fails.
 *
 * @code
 * struct cls_assets *assets;
 * cls_assets_create(&assets, mem, api);
 * // Use assets.
 * cls_assets_destroy(assets);
 * @endcode
 */
cls_error cls_assets_create(struct cls_assets **assets, struct cls_mem *mem,
                            struct cls_renderer_api *api);

/**
 * @brief Destroys an asset store.
 *
 * Destroys all loaded fonts, shuts down FreeType, and releases the internal
 * asset tables.
 *
 * @note Meshes, shaders, and textures are not destroyed individually. Perform
 * any required GPU cleanup before calling this function.
 *
 * @param[in] assets Asset store to destroy.
 */
void cls_assets_destroy(struct cls_assets *assets);

/**
 * @brief Loads a font into the asset store.
 *
 * Loads the font, creates its glyph atlas, uploads the atlas texture, and
 * stores both resources. Does nothing if the font is already loaded.
 *
 * @param[in] assets     Asset store.
 * @param[in] id         Font identifier.
 * @param[in] font_path  Path to the font file, relative to the font directory.
 * @param[in] pixel_size Pixel size used to rasterize the glyph atlas.
 */
void cls_assets_font_add(struct cls_assets *assets, cls_font_id id,
                         const char *font_path, int pixel_size);

/**
 * @brief Retrieves a font by its identifier.
 *
 * @param[out] font   Pointer to the font.
 * @param[in]  assets Asset store to search.
 * @param[in]  id     Font identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `font` or `assets` is NULL.
 * @retval (error)     If the lookup fails.
 *
 * @note On success, `*font` may be NULL if no font is registered under `id`.
 */
cls_error cls_assets_font_get(struct cls_font **font,
                              const struct cls_assets *assets, cls_font_id id);

/**
 * @brief Loads a shader into the asset store.
 *
 * Loads the shader source from disk, compiles it, and stores the resulting
 * shader. Does nothing if the shader is already loaded.
 *
 * @param[in] assets      Asset store.
 * @param[in] id          Shader identifier.
 * @param[in] shader_path Base shader path, relative to the shader directory.
 *                        The `.vert` and `.frag` extensions are appended
 *                        automatically.
 */
void cls_assets_shader_add(struct cls_assets *assets, cls_shader_id id,
                           const char *shader_path);

/**
 * @brief Retrieves a shader by its identifier.
 *
 * @param[out] shader Pointer to the shader.
 * @param[in]  assets Asset store to search.
 * @param[in]  id     Hashed shader identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `shader` or `assets` is NULL, or `shader_id` is 0.
 * @retval (error)     If the lookup fails.
 *
 * @note On success, `*shader` may be NULL if no shader is registered under
 * `shader_id`.
 */
cls_error cls_assets_shader_get(struct cls_shader **shader,
                                const struct cls_assets *assets,
                                cls_shader_id id);

/**
 * @brief Loads a texture into the asset store.
 *
 * Loads the texture from disk, uploads it with the specified filter and wrap
 * modes, and stores it. Does nothing if the texture is already loaded.
 *
 * @param[in] assets         Asset store.
 * @param[in] id             Texture identifier.
 * @param[in] texture2d_path Path to the texture file, relative to the texture
 *                           directory.
 * @param[in] filter         Texture filtering mode.
 * @param[in] wrap           Texture wrapping mode.
 */
void cls_assets_texture2d_add(struct cls_assets *assets, cls_texture2d_id id,
                              const char *texture2d_path,
                              enum cls_texture2d_filter filter,
                              enum cls_texture2d_wrap wrap);

/**
 * @brief Retrieves a texture by its identifier.
 *
 * If `texture2d_id` is 0, returns the default blank texture. If no texture is
 * registered under `texture2d_id`, returns the default missing texture.
 *
 * @param[out] texture Pointer to the texture.
 * @param[in]  assets  Asset store to search.
 * @param[in]  id      Hashed texture identifier. Pass 0 to retrieve the default
 *                     blank texture.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `texture` or `assets` is NULL.
 * @retval (error)     If retrieving the requested texture or a fallback
 *                     texture fails.
 */
cls_error cls_assets_texture2d_get(struct cls_texture2d **texture,
                                   const struct cls_assets *assets,
                                   cls_texture2d_id id);

/**
 * @brief Creates a mesh and adds it to the asset store.
 *
 * Creates a mesh from the supplied vertex and index data and stores it. Does
 * nothing if the mesh is already loaded.
 *
 * @param[in] assets       Asset store.
 * @param[in] id           Mesh identifier.
 * @param[in] vertices     Vertex data.
 * @param[in] vertex_count Number of vertices in `vertices`.
 * @param[in] indices      Index data.
 * @param[in] index_count  Number of indices in `indices`.
 */
void cls_assets_mesh_add(struct cls_assets *assets, cls_gl_mesh_id id,
                         const struct cls_vertex *vertices, size_t vertex_count,
                         const unsigned int *indices, size_t index_count);

/**
 * @brief Retrieves a mesh by its identifier.
 *
 * @param[out] mesh    Pointer to the mesh.
 * @param[in]  assets  Asset store to search.
 * @param[in]  mesh_id Hashed mesh identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `mesh` or `assets` is NULL, or `mesh_id` is 0.
 * @retval (error)     If the lookup fails.
 *
 * @note On success, `*mesh` may be NULL if no mesh is registered under
 * `mesh_id`.
 */
cls_error cls_assets_mesh_get(struct cls_gl_mesh **mesh,
                              const struct cls_assets *assets, u32 mesh_id);

/** @} */

#endif // CLS_ASSETS_H
