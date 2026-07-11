#include "cls/ecs/component/components.h"
#include <cglm/ivec4.h>
#include <cglm/vec2.h>
#include <cls/app/assets.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/io/font.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/xxhash32.h>
#include <glad/gl.h>
#include <stdio.h>

int cls_preset_group_despawn(cls_entity e, struct cls_ecs_world *world) {
    void *g_ptr = NULL;
    int error = cls_ecs_world_component_get(&g_ptr, world, e, CLS_COMP_GROUP);
    if (error)
        return error;

    struct group *g = g_ptr;
    struct cls_ecs_world_query *query = NULL;
    error = cls_ecs_world_query_create(&query, world, 1,
                                       (const char *[]){CLS_COMP_GROUP});
    if (error)
        return error;

    if (!query)
        return CLS_NULLPTR;

    u32 id = g->grp_id;
    cls_entity e_next = CLS_ENTITY_MAX;
    void *comps[1] = {NULL};
    while (cls_ecs_world_query_next(&e_next, comps, query) == CLS_SUCCESS &&
           e_next != CLS_ENTITY_MAX) {
        // TODO: Need to cache the deleted entities or something to prevent
        // deleting entities on error (delete then error problem)
        struct group *grp = comps[0];
        if (!grp)
            continue;

        if (grp->grp_id == id) {
            error = cls_ecs_world_entity_remove(world, e_next);
            if (error)
                continue;
        }
    }

    cls_ecs_world_query_destroy(query);
    return CLS_SUCCESS;
}

int cls_preset_camera_ortho_spawn(cls_entity *camera,
                                  struct cls_ecs_world *world, vec3 pos,
                                  float left, float right, float bottom,
                                  float top, float zoom, float near_clip,
                                  float far_clip, bool y_down, int layer,
                                  bool active) {
    struct camera cam_comp = {.type = CAMERA_ORTHO,
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
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]}, .dirty = true};

    glm_mat4_identity(cam_comp.view);
    glm_mat4_identity(cam_comp.projection);

    cls_entity root = CLS_ENTITY_MAX;
    int error = cls_ecs_world_entity_add(&root, world);
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
                                            &(struct camera_active){0});
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

int cls_preset_renderable_spawn(cls_entity *ren, struct cls_ecs_world *world,
                                const char *mesh_id, const char *shader_id,
                                const char *texture2d_id, vec3 pos, vec3 scale,
                                float rot_angle, vec2 uv_offset, vec2 uv_scale,
                                ivec4 tint, bool visible, bool depth_test,
                                bool depth_write, bool blending, int blend_src,
                                int blend_dest) {
    struct renderable ren_comp = {.state = {.depth_test = depth_test,
                                            .depth_write = depth_write,
                                            .blending = blending,
                                            .blend_src = blend_src,
                                            .blend_dest = blend_dest},
                                  .uv_offset = {uv_offset[0], uv_offset[1]},
                                  .uv_scale = {uv_scale[0], uv_scale[1]},
                                  .opacity = 1.0f,
                                  .visible = visible};
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]},
                           .scale = {scale[0], scale[1], scale[2]},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = rot_angle,
                           .dirty = true};

    glm_ivec4_copy(tint, ren_comp.tint);

    int error = cls_xxhash32(&ren_comp.mesh_id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return error;

    error = cls_xxhash32(&ren_comp.shader_id, shader_id, strlen(shader_id), 0);
    if (error)
        return error;

    error = cls_xxhash32(&ren_comp.texture_id, texture2d_id,
                         strlen(texture2d_id), 0);
    if (error)
        return error;

    cls_entity root = CLS_ENTITY_MAX;
    error = cls_ecs_world_entity_add(&root, world);
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

int cls_preset_rect_spawn(cls_entity *rect, struct cls_ecs_world *world,
                          const char *texture2d_id, vec2 pos, vec2 scale,
                          float rot_angle, float z_index, ivec4 tint,
                          vec2 uv_offset, vec2 uv_scale, bool visible) {
    return cls_preset_renderable_spawn(
        rect, world, "quad", "mesh", texture2d_id,
        (vec3){pos[0], pos[1], z_index}, (vec3){scale[0], scale[1], 1.0f},
        rot_angle, uv_offset, uv_scale, tint, visible, true, true, false,
        GL_ONE, GL_ZERO);
}

int cls_preset_sprite_spawn(cls_entity *sprite, struct cls_ecs_world *world,
                            const char *texture2d_id, vec2 pos, vec2 scale,
                            float rot_angle, float z_index, ivec4 tint,
                            vec2 uv_offset, vec2 uv_scale, bool visible) {
    return cls_preset_renderable_spawn(
        sprite, world, "quad", "sprite", texture2d_id,
        (vec3){pos[0], pos[1], z_index}, (vec3){scale[0], scale[1], 1.0f},
        rot_angle, uv_offset, uv_scale, tint, visible, true, false, true,
        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int cls_preset_image_button_spawn(cls_entity *button,
                                  struct cls_ecs_world *world, const char *id,
                                  const char *img_id, vec2 pos, float z_index,
                                  vec2 scale, vec2 uv_offset, vec2 uv_scale,
                                  ivec4 img_tint, bool visible) {
    struct ui ui = {.interactable = true};
    struct button button_comp = {0};

    u32 id_hash = 0;
    int error = cls_xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    cls_entity root = CLS_ENTITY_MAX;
    error = cls_preset_rect_spawn(&root, world, "", pos, scale, 0.0f, z_index,
                                  (ivec4){0, 0, 0, 255}, uv_offset, uv_scale,
                                  visible);
    if (error)
        return error;

    const char *root_id = "root";
    u32 root_hash = 0;
    error = cls_xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(
        world, root, CLS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_BUTTON_GROUP,
                                        &(struct button_group){0});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_UI, &ui);
    if (error)
        goto cleanup;

    error =
        cls_ecs_world_component_add(world, root, CLS_COMP_BUTTON, &button_comp);
    if (error)
        goto cleanup;

    cls_entity sprite = CLS_ENTITY_MAX;
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
        &(struct group){.grp_id = id_hash, .user_id = sprite_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, sprite, CLS_COMP_BUTTON_GROUP,
                                        &(struct button_group){0});
    if (error)
        goto cleanup;

    if (button)
        *button = root;

    return CLS_SUCCESS;
cleanup:
    // TODO: Delete spawned entities
    return error;
}

int cls_preset_label_spawn(cls_entity *label, struct cls_ecs_world *world,
                           const char *id, vec2 pos, float z_index,
                           const char *text, int font_size, const char *font_id,
                           bool visible, ivec4 tint) {
    if (!world || !id || !text || !font_id)
        return CLS_NULLPTR;

    struct label label_comp = {.font_size = font_size, .visible = visible};
    glm_ivec4_copy((int *)tint, label_comp.tint);

    int ret = snprintf(label_comp.text, sizeof(label_comp.text), "%s", text);
    if (ret < 0)
        return CLS_FAILURE;

    u32 id_hash = 0;
    int error = cls_xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    error = cls_xxhash32(&label_comp.font_id, font_id, strlen(font_id), 0);
    if (error)
        return error;

    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .scale = {1.0f, 1.0f, 1.0f},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = 0.0f};

    cls_entity root = CLS_ENTITY_MAX;
    error = cls_ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = cls_ecs_world_component_add(world, root, CLS_COMP_GROUP,
                                        &(struct group){.grp_id = id_hash});
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
