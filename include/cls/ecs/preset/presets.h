/**
 * @file cls/ecs/preset/presets.h
 * @brief Default presets for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/preset/presets.c
 */

#ifndef CLS_PRESETS_H
#define CLS_PRESETS_H

#include <cls/app/assets.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>

/**
 * @defgroup presets Presets
 * @ingroup ecs
 * @brief Default preset helpers.
 * @{
 */

/* Forward declarations. */
struct cls_assets;
struct cls_ecs_world;

/**
 * @struct cls_preset_ui_image_button
 * @brief UI image button preset.
 */
struct cls_preset_ui_image_button {
    struct cls_renderable ren;
    struct cls_transform tf;
    struct cls_ui ui;
    struct cls_button button;
    struct cls_button_group grp;
};

/**
 * @struct cls_preset_ui_label
 * @brief UI label preset.
 */
struct cls_preset_ui_label {
    struct cls_renderable ren;
    struct cls_transform tf;
    struct cls_ui ui;
    struct cls_label label;
};

/**
 * @brief Despawns an entity group.
 *
 * Despawns every entity in the same group as `e`.
 *
 * @param[in] e     Entity.
 * @param[in] world World.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     Error if retrieving the entity group or despawning the
 *                     group fails.
 */
cls_error cls_preset_group_despawn(cls_entity e, struct cls_ecs_world *world);

/**
 * @brief Spawns an orthographic camera.
 *
 * Spawns a camera entity with an orthographic projection.
 *
 * @param[out] camera    Camera entity.
 * @param[in]  world     World.
 * @param[in]  pos       Position.
 * @param[in]  left      Left clipping plane.
 * @param[in]  right     Right clipping plane.
 * @param[in]  bottom    Bottom clipping plane.
 * @param[in]  top       Top clipping plane.
 * @param[in]  zoom      Zoom factor.
 * @param[in]  near_clip Near clipping plane.
 * @param[in]  far_clip  Far clipping plane.
 * @param[in]  y_down    Whether the y axis increases downward.
 * @param[in]  layer     Render layer.
 * @param[in]  active    Whether the camera starts active.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     Error if creating the camera fails.
 */
cls_error cls_preset_camera_ortho_spawn(cls_entity *camera,
                                        struct cls_ecs_world *world, vec3 pos,
                                        float left, float right, float bottom,
                                        float top, float zoom, float near_clip,
                                        float far_clip, bool y_down, int layer,
                                        bool active);

/**
 * @brief Spawns a renderable.
 *
 * Spawns a renderable entity.
 *
 * @param[out] ren          Entity.
 * @param[in]  world        World.
 * @param[in]  mesh_id      Mesh identifier.
 * @param[in]  shader_id    Shader identifier.
 * @param[in]  texture2d_id Texture identifier.
 * @param[in]  pos          Position.
 * @param[in]  scale        Scale.
 * @param[in]  rot_angle    Rotation angle.
 * @param[in]  uv_offset    Texture coordinate offset.
 * @param[in]  uv_scale     Texture coordinate scale.
 * @param[in]  tint         Tint color.
 * @param[in]  visible      Whether the entity starts visible.
 * @param[in]  depth_test   Whether depth testing is enabled.
 * @param[in]  depth_write  Whether depth writes are enabled.
 * @param[in]  blending     Whether blending is enabled.
 * @param[in]  blend_src    Source blend factor.
 * @param[in]  blend_dest   Destination blend factor.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     Error if creating the renderable fails.
 */
cls_error cls_preset_renderable_spawn(
    cls_entity *ren, struct cls_ecs_world *world, cls_gl_mesh_id mesh_id,
    cls_shader_id shader_id, cls_texture2d_id texture2d_id, vec3 pos,
    vec3 scale, float rot_angle, vec2 uv_offset, vec2 uv_scale, ivec4 tint,
    bool visible, bool depth_test, bool depth_write, bool blending,
    int blend_src, int blend_dest);

/**
 * @brief Spawns a rectangle.
 *
 * Spawns a 2D rectangle entity.
 *
 * @param[out] rect         Entity.
 * @param[in]  world        World.
 * @param[in]  texture2d_id Texture identifier.
 * @param[in]  pos          Position.
 * @param[in]  scale        Scale.
 * @param[in]  rot_angle    Rotation angle.
 * @param[in]  z_index      Depth.
 * @param[in]  tint         Tint color.
 * @param[in]  uv_offset    Texture coordinate offset.
 * @param[in]  uv_scale     Texture coordinate scale.
 * @param[in]  visible      Whether the entity starts visible.
 *
 * @return CLS_SUCCESS On success.
 * @retval (error) Error propagated from cls_preset_renderable_spawn().
 */
cls_error cls_preset_rect_spawn(cls_entity *rect, struct cls_ecs_world *world,
                                cls_texture2d_id texture2d_id, vec2 pos,
                                vec2 scale, float rot_angle, float z_index,
                                ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                                bool visible);

/**
 * @brief Spawns a sprite.
 *
 * Spawns a 2D sprite entity.
 *
 * @param[out] sprite       Entity.
 * @param[in]  world        World.
 * @param[in]  texture2d_id Texture identifier.
 * @param[in]  pos          Position.
 * @param[in]  scale        Scale.
 * @param[in]  rot_angle    Rotation angle.
 * @param[in]  z_index      Depth.
 * @param[in]  tint         Tint color.
 * @param[in]  uv_offset    Texture coordinate offset.
 * @param[in]  uv_scale     Texture coordinate scale.
 * @param[in]  visible      Whether the entity starts visible.
 *
 * @return CLS_SUCCESS On success.
 * @retval (error) Error propagated from cls_preset_renderable_spawn().
 */
cls_error cls_preset_sprite_spawn(cls_entity *sprite,
                                  struct cls_ecs_world *world,
                                  cls_texture2d_id texture2d_id, vec2 pos,
                                  vec2 scale, float rot_angle, float z_index,
                                  ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                                  bool visible);

/**
 * @brief Spawns an image button.
 *
 * Spawns an image button entity.
 *
 * @param[out] button     Button entity.
 * @param[in]  world      World.
 * @param[in]  id         Button identifier.
 * @param[in]  img_id     Image identifier.
 * @param[in]  pos        Position.
 * @param[in]  z_index    Depth.
 * @param[in]  scale      Scale.
 * @param[in]  uv_offset  Texture coordinate offset.
 * @param[in]  uv_scale   Texture coordinate scale.
 * @param[in]  img_tint   Image tint.
 * @param[in]  visible    Whether the button starts visible.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` or `id` is NULL.
 * @retval (error)     Error if creating the button fails.
 */
cls_error cls_preset_image_button_spawn(
    cls_entity *button, struct cls_ecs_world *world, const char *id,
    cls_texture2d_id image_id, vec2 pos, float z_index, vec2 scale,
    vec2 uv_offset, vec2 uv_scale, ivec4 image_tint, bool visible);

/**
 * @brief Spawns a text label.
 *
 * Spawns a label entity.
 *
 * @param[out] label     Label entity.
 * @param[in]  world     World.
 * @param[in]  id        Label identifier.
 * @param[in]  pos       Position.
 * @param[in]  z_index   Depth.
 * @param[in]  text      Label text.
 * @param[in]  font_size Font size.
 * @param[in]  font_id   Font identifier.
 * @param[in]  visible   Whether the label starts visible.
 * @param[in]  tint      Tint color.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world`, `id`, or `text` is NULL.
 * @retval CLS_FAILURE If formatting the label text fails.
 * @retval (error)     Error if creating the label fails.
 */
cls_error cls_preset_label_spawn(cls_entity *label, struct cls_ecs_world *world,
                                 const char *id, vec2 pos, float z_index,
                                 const char *text, int font_size,
                                 cls_font_id font_id, bool visible, ivec4 tint);

/** @} */

#endif // CLS_PRESETS_H
