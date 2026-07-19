/**
 * @file cls/ecs/preset/presets.c
 * @brief Default presets for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/preset/presets.h
 */

#include "cls/ecs/component/components.h"
#include "cls/util/error.h"
#include <cglm/ivec4.h>
#include <cglm/vec2.h>
#include <cls/app/assets.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/io/font.h>
#include <cls/util/array.h>
#include <cls/util/logger.h>
#include <cls/util/xxhash32.h>
#include <glad/gl.h>
#include <stdio.h>

static const size_t START_CAPACITY = 8;

cls_error cls_preset_group_despawn(cls_entity e, struct cls_ecs_world *world) {
    if (!world)
        return CLS_NULLPTR;

    void *g_ptr = NULL;
    cls_error error =
        cls_ecs_world_component_get(&g_ptr, world, e, CLS_COMP_GROUP);
    if (error)
        return error;

    struct cls_group *g = g_ptr;
    u32 id = g->grp_id;

    struct cls_ecs_world_query *query = NULL;
    error = cls_ecs_world_query_create(&query, world, 1,
                                       (const cls_component[]){CLS_COMP_GROUP});
    if (error)
        return error;

    if (!query)
        return CLS_NULLPTR;

    struct cls_array *matches = NULL;
    error = cls_array_create(&matches, START_CAPACITY, sizeof(cls_entity));
    if (error)
        goto cleanup;

    // Find matches without mutating the world
    cls_entity e_next = CLS_ENTITY_MAX;
    void *comps[1] = {NULL};
    while (cls_ecs_world_query_next(&e_next, comps, query) == CLS_SUCCESS &&
           e_next != CLS_ENTITY_MAX) {
        struct cls_group *grp = comps[0];
        if (!grp)
            continue;

        if (grp->grp_id == id) {
            error = cls_array_push(&matches, &e_next);
            if (error)
                goto cleanup;
        }
    }

    cls_ecs_world_query_destroy(query);

    // Remove found matches
    size_t matches_len = 0;
    error = cls_array_length_get(&matches_len, matches);
    if (error) {
        cls_array_destroy(matches);
        return error;
    }

    for (size_t i = 0; i < matches_len; ++i) {
        cls_entity match = CLS_ENTITY_MAX;
        error = cls_array_elem_get_cpy(&match, matches, i);
        if (error)
            continue;

        error = cls_ecs_world_entity_remove(world, match);
        if (error)
            continue;
    }

    cls_array_destroy(matches);
    return CLS_SUCCESS;

cleanup:
    cls_array_destroy(matches);
    cls_ecs_world_query_destroy(query);
    return error;
}

cls_error cls_preset_camera_ortho_spawn(cls_entity *camera,
                                        struct cls_ecs_world *world, vec3 pos,
                                        float left, float right, float bottom,
                                        float top, float zoom, float near_clip,
                                        float far_clip, bool y_down, int layer,
                                        bool active) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_camera cam_comp = {.type = CLS_CAMERA_ORTHO,
                                  .ortho.left = left,
                                  .ortho.right = right,
                                  .ortho.bottom = bottom,
                                  .ortho.top = top,
                                  .ortho.y_down = y_down,
                                  .zoom = zoom,
                                  .near_clip = near_clip,
                                  .far_clip = far_clip,
                                  .layer = layer,
                                  .dirty = true};
    struct cls_transform tf = {.pos = {pos[0], pos[1], pos[2]}, .dirty = true};

    glm_mat4_identity(cam_comp.view);
    glm_mat4_identity(cam_comp.projection);

    cls_entity root = CLS_ENTITY_MAX;
    cls_error error = cls_ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error =
        cls_ecs_world_component_add(world, root, CLS_COMP_CAMERA, &cam_comp);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_TRANSFORM, &tf);
    if (error)
        goto cleanup;

    if (active) {
        error = cls_ecs_world_component_add(world, root, CLS_COMP_CAMERA_ACTIVE,
                                            &(struct cls_camera_active){0});
        if (error)
            goto cleanup;
    }

    if (camera)
        *camera = root;

    return CLS_SUCCESS;

cleanup:
    if (root != CLS_ENTITY_MAX)
        cls_ecs_world_entity_remove(world, root);

    return error;
}

cls_error cls_preset_renderable_spawn(
    cls_entity *ren, struct cls_ecs_world *world, cls_gl_mesh_id mesh_id,
    cls_shader_id shader_id, cls_texture2d_id texture2d_id, vec3 pos,
    vec3 scale, float rot_angle, vec2 uv_offset, vec2 uv_scale, ivec4 tint,
    bool visible, bool depth_test, bool depth_write, bool blending,
    int blend_src, int blend_dest) {
    if (!world)
        return CLS_NULLPTR;

    struct cls_renderable ren_comp = {.state = {.depth_test = depth_test,
                                                .depth_write = depth_write,
                                                .blending = blending,
                                                .blend_src = blend_src,
                                                .blend_dest = blend_dest},
                                      .mesh_id = mesh_id,
                                      .shader_id = shader_id,
                                      .texture_id = texture2d_id,
                                      .uv_offset = {uv_offset[0], uv_offset[1]},
                                      .uv_scale = {uv_scale[0], uv_scale[1]},
                                      .opacity = 1.0f,
                                      .visible = visible};
    struct cls_transform tf = {.pos = {pos[0], pos[1], pos[2]},
                               .scale = {scale[0], scale[1], scale[2]},
                               .rot_axis = {0.0f, 0.0f, 1.0f},
                               .rot_angle = rot_angle,
                               .dirty = true};

    glm_ivec4_copy(tint, ren_comp.tint);

    cls_entity root = CLS_ENTITY_MAX;
    cls_error error = cls_ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_RENDERABLE,
                                        &ren_comp);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_TRANSFORM, &tf);
    if (error)
        goto cleanup;

    if (ren)
        *ren = root;

    return CLS_SUCCESS;

cleanup:
    if (root != CLS_ENTITY_MAX)
        cls_ecs_world_entity_remove(world, root);

    return error;
}

cls_error cls_preset_rect_spawn(cls_entity *rect, struct cls_ecs_world *world,
                                cls_texture2d_id texture2d_id, vec2 pos,
                                vec2 scale, float rot_angle, float z_index,
                                ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                                bool visible) {
    return cls_preset_renderable_spawn(
        rect, world, CLS_MESH_QUAD, CLS_SHADER_MESH, texture2d_id,
        (vec3){pos[0], pos[1], z_index}, (vec3){scale[0], scale[1], 1.0f},
        rot_angle, uv_offset, uv_scale, tint, visible, true, true, false,
        GL_ONE, GL_ZERO);
}

cls_error cls_preset_sprite_spawn(cls_entity *sprite,
                                  struct cls_ecs_world *world,
                                  cls_texture2d_id texture2d_id, vec2 pos,
                                  vec2 scale, float rot_angle, float z_index,
                                  ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                                  bool visible) {
    return cls_preset_renderable_spawn(
        sprite, world, CLS_MESH_QUAD, CLS_SHADER_SPRITE, texture2d_id,
        (vec3){pos[0], pos[1], z_index}, (vec3){scale[0], scale[1], 1.0f},
        rot_angle, uv_offset, uv_scale, tint, visible, true, false, true,
        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

cls_error cls_preset_image_button_spawn(cls_entity *button,
                                        struct cls_ecs_world *world,
                                        const char *id, cls_texture2d_id img_id,
                                        vec2 pos, float z_index, vec2 scale,
                                        vec2 uv_offset, vec2 uv_scale,
                                        ivec4 img_tint, bool visible) {
    if (!world || !id || !img_id)
        return CLS_NULLPTR;

    struct cls_ui ui = {.interactable = true};
    struct cls_button button_comp = {0};

    u32 id_hash = 0;
    cls_error error = cls_xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    cls_entity root = CLS_ENTITY_MAX;
    cls_entity sprite = CLS_ENTITY_MAX;
    error = cls_preset_rect_spawn(&root, world, CLS_TEXTURE2D_BLANK, pos, scale,
                                  0.0f, z_index, (ivec4){0, 0, 0, 255},
                                  uv_offset, uv_scale, visible);
    if (error)
        return error;

    const char *root_id = "root";
    u32 root_hash = 0;
    error = cls_xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(
        world, root, CLS_COMP_GROUP,
        &(struct cls_group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_BUTTON_GROUP,
                                        &(struct cls_button_group){0});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_UI, &ui);
    if (error)
        goto cleanup;

    error =
        cls_ecs_world_component_add(world, root, CLS_COMP_BUTTON, &button_comp);
    if (error)
        goto cleanup;

    error = cls_preset_sprite_spawn(&sprite, world, img_id, pos, scale, 0.0f,
                                    z_index + 0.001f, img_tint, uv_offset,
                                    uv_scale, visible);
    if (error)
        goto cleanup;

    const char *sprite_id = "sprite";
    u32 sprite_hash = 0;
    error = cls_xxhash32(&sprite_hash, sprite_id, strlen(sprite_id), 0);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(
        world, sprite, CLS_COMP_GROUP,
        &(struct cls_group){.grp_id = id_hash, .user_id = sprite_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, sprite, CLS_COMP_BUTTON_GROUP,
                                        &(struct cls_button_group){0});
    if (error)
        goto cleanup;

    if (button)
        *button = root;

    return CLS_SUCCESS;

cleanup:
    if (root != CLS_ENTITY_MAX)
        cls_ecs_world_entity_remove(world, root);

    if (sprite != CLS_ENTITY_MAX)
        cls_ecs_world_entity_remove(world, sprite);

    return error;
}

cls_error cls_preset_label_spawn(cls_entity *label, struct cls_ecs_world *world,
                                 const char *id, vec2 pos, float z_index,
                                 const char *text, int font_size,
                                 cls_font_id font_id, bool visible,
                                 ivec4 tint) {
    if (!world || !id || !text)
        return CLS_NULLPTR;

    struct cls_label label_comp = {
        .font_size = font_size, .font_id = font_id, .visible = visible};
    glm_ivec4_copy((int *)tint, label_comp.tint);

    int ret = snprintf(label_comp.text, sizeof(label_comp.text), "%s", text);
    if (ret < 0)
        return CLS_FAILURE;

    u32 id_hash = 0;
    cls_error error = cls_xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    struct cls_transform tf = {.pos = {pos[0], pos[1], z_index},
                               .scale = {1.0f, 1.0f, 1.0f},
                               .rot_axis = {0.0f, 0.0f, 1.0f},
                               .rot_angle = 0.0f};

    cls_entity root = CLS_ENTITY_MAX;
    error = cls_ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_GROUP,
                                        &(struct cls_group){.grp_id = id_hash});
    if (error)
        goto cleanup;

    error =
        cls_ecs_world_component_add(world, root, CLS_COMP_LABEL, &label_comp);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_TRANSFORM, &tf);
    if (error)
        goto cleanup;

    if (label)
        *label = root;

    return CLS_SUCCESS;

cleanup:
    cls_ecs_world_entity_remove(world, root);
    return error;
}
