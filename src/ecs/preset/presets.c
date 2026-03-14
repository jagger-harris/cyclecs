#include <cglm/ivec4.h>
#include <cglm/vec2.h>
#include <cls/app/assets.h>
#include <cls/ecs/preset/presets.h>
#include <cls/io/ffont.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/xxhash32.h>
#include <stdio.h>

int preset_camera_ortho_spawn(entity *camera, struct ecs_world *world, vec3 pos,
                              float left, float right, float bottom, float top,
                              float zoom, float near_clip, float far_clip,
                              bool y_down, bool active) {
    struct camera cam_data = {.type = CAMERA_ORTHO,
                              .ortho.left = left,
                              .ortho.right = right,
                              .ortho.bottom = bottom,
                              .ortho.top = top,
                              .ortho.y_down = y_down,
                              .zoom = zoom,
                              .near_clip = near_clip,
                              .far_clip = far_clip,
                              .update = true};
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]}};

    glm_mat4_identity(cam_data.view);
    glm_mat4_identity(cam_data.projection);

    entity root = ENTITY_MAX;
    int error = ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error =
        ecs_world_component_add(world, root, CLS_ECS_COMP_CAMERA, &cam_data);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_TRANSFORM, &tf);
    if (error)
        goto cleanup;

    if (active) {
        error = ecs_world_component_add(world, root, CLS_ECS_COMP_CAMERA_ACTIVE,
                                        &(struct camera_active){0});
        if (error)
            goto cleanup;
    }

    if (camera)
        *camera = root;

    return CLS_SUCCESS;

cleanup:
    if (root != U32_MAX)
        ecs_world_entity_remove(world, root);

    return error;
}

int preset_renderable_spawn(entity *ren, struct ecs_world *world,
                            const char *mesh_id, const char *shader_id,
                            const char *texture2d_id, vec3 pos, vec3 scale,
                            float rot_angle, vec2 uv_offset, vec2 uv_scale,
                            ivec4 tint, bool visible, bool transparent) {
    struct renderable ren_data = {.uv_offset = {uv_offset[0], uv_offset[1]},
                                  .uv_scale = {uv_scale[0], uv_scale[1]},
                                  .opacity = 1.0f,
                                  .visible = visible,
                                  .transparent = transparent};
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]},
                           .scale = {scale[0], scale[1], scale[2]},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = rot_angle};

    glm_ivec4_copy(tint, ren_data.tint);

    int error = xxhash32(&ren_data.mesh_id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return error;

    error = xxhash32(&ren_data.shader_id, shader_id, strlen(shader_id), 0);
    if (error)
        return error;

    error =
        xxhash32(&ren_data.texture_id, texture2d_id, strlen(texture2d_id), 0);
    if (error)
        return error;

    entity root = ENTITY_MAX;
    error = ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_RENDERABLE,
                                    &ren_data);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_TRANSFORM, &tf);
    if (error)
        goto cleanup;

    if (ren)
        *ren = root;

    return CLS_SUCCESS;
cleanup:
    if (root != U32_MAX)
        ecs_world_entity_remove(world, root);

    return error;
}

int preset_rect_spawn(entity *rect, struct ecs_world *world,
                      const char *texture2d_id, vec2 pos, vec2 scale,
                      float rot_angle, float z_index, ivec4 tint,
                      vec2 uv_offset, vec2 uv_scale, bool visible) {
    return preset_renderable_spawn(rect, world, "quad", "mesh", texture2d_id,
                                   (vec3){pos[0], pos[1], z_index},
                                   (vec3){scale[0], scale[1], 1.0f}, rot_angle,
                                   uv_offset, uv_scale, tint, visible, false);
}

int preset_sprite_spawn(entity *sprite, struct ecs_world *world,
                        const char *texture2d_id, vec2 pos, vec2 scale,
                        float rot_angle, float z_index, ivec4 tint,
                        vec2 uv_offset, vec2 uv_scale, bool visible) {
    return preset_renderable_spawn(
        sprite, world, "quad", "sprite", texture2d_id,
        (vec3){pos[0], pos[1], z_index}, (vec3){scale[0], scale[1], 1.0f},
        rot_angle, uv_offset, uv_scale, tint, visible, true);
}

int preset_ui_image_button_spawn(entity *button, struct ecs_world *world,
                                 const char *id, const char *img_id, vec2 pos,
                                 float z_index, vec2 scale, vec2 uv_offset,
                                 vec2 uv_scale, ivec4 img_tint, bool visible) {
    struct ui_base base = {.interactable = true};
    struct ui_button button_data = {0};

    u32 id_hash = 0;
    int error = xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    entity root = ENTITY_MAX;
    error =
        preset_rect_spawn(&root, world, "", pos, scale, 0.0f, z_index,
                          (ivec4){0, 0, 0, 255}, uv_offset, uv_scale, visible);
    if (error)
        return error;

    const char *root_id = "root";
    u32 root_hash = 0;
    error = xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(
        world, root, CLS_ECS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_BUTTON_GROUP,
                                    &(struct ui_button_group){0});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_BASE, &base);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_BUTTON,
                                    &button_data);
    if (error)
        goto cleanup;

    entity sprite = ENTITY_MAX;
    error = preset_sprite_spawn(&sprite, world, img_id, pos, scale, 0.0f,
                                z_index + 0.01f, img_tint, uv_offset, uv_scale,
                                visible);
    if (error)
        goto cleanup;

    const char *sprite_id = "sprite";
    u32 sprite_hash = 0;
    error = xxhash32(&sprite_hash, sprite_id, strlen(sprite_id), 0);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(
        world, sprite, CLS_ECS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = sprite_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, sprite, CLS_ECS_COMP_UI_BUTTON_GROUP,
                                    &(struct ui_button_group){0});
    if (error)
        goto cleanup;

    if (button)
        *button = root;

    return CLS_SUCCESS;
cleanup:
    // TODO: Delete spawned entities
    return error;
}

int preset_ui_label_spawn(entity *label, struct ecs_world *world,
                          struct assets *assets, const char *id, vec2 pos,
                          float z_index, const char *text, int font_size,
                          const char *font_id, bool visible, ivec4 tint) {
    struct ui_base base = {.interactable = false};
    struct ui_label label_data = {.font_size = font_size};
    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = 0.0f};

    int ret = snprintf(label_data.text, sizeof(label_data.text), "%s", text);
    if (ret < 0)
        return CLS_FAILURE;

    u32 id_hash = 0;
    int error = xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    error = xxhash32(&label_data.font_id, font_id, strlen(font_id), 0);
    if (error)
        return error;

    const struct ffont *font = NULL;
    error = assets_font_get(&font, assets, label_data.font_id);
    if (error)
        return error;

    if (!font) {
        LOGGER_LOG(LOGGER_ERROR, "%s", "Missing font for ui label");
        return CLS_NULLPTR;
    }

    entity root = ENTITY_MAX;
    error = ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_GROUP,
                                    &(struct group){.grp_id = id_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_LABEL_GROUP,
                                    &(struct ui_label_group){0});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_BASE, &base);
    if (error)
        return error;

    error = ecs_world_component_add(world, root, CLS_ECS_COMP_UI_LABEL,
                                    &label_data);
    if (error)
        return error;

    float scale = (float)label_data.font_size / (float)font->pixel_size;
    float cursor_x = tf.pos[0];
    float cursor_y = tf.pos[1];
    size_t text_length = strlen(label_data.text);

    for (size_t i = 0; i < text_length; ++i) {
        u8 c = (u8)label_data.text[i];
        if (c < FFONT_CHAR_START || c > FFONT_CHAR_END)
            continue;

        const struct fglyph *glyph_data = &font->glyphs[c - FFONT_CHAR_START];
        if (glyph_data->width == 0 || glyph_data->height == 0) {
            cursor_x += (float)glyph_data->advance * scale;
            continue;
        }

        float pos_x = cursor_x + (float)glyph_data->bearing_x * scale +
                      ((float)glyph_data->width * scale * 0.5f);
        float pos_y = cursor_y - (float)glyph_data->bearing_y * scale +
                      ((float)glyph_data->height * scale * 0.5f);

        vec3 glyph_pos = {pos_x, pos_y, tf.pos[2]};
        vec3 glyph_scale = {(float)glyph_data->width * scale,
                            (float)glyph_data->height * scale, 1.0f};

        entity glyph = ENTITY_MAX;
        error = preset_renderable_spawn(
            &glyph, world, "quad", "font", font_id, glyph_pos, glyph_scale,
            0.0f,
            (vec2){(float)glyph_data->atlas_x / (float)font->atlas_width,
                   (float)glyph_data->atlas_y / (float)font->atlas_height},
            (vec2){(float)glyph_data->width / (float)font->atlas_width,
                   (float)glyph_data->height / (float)font->atlas_height},
            tint, visible, true);
        if (error)
            goto cleanup;

        error = ecs_world_component_add(world, glyph, CLS_ECS_COMP_GROUP,
                                        &(struct group){.grp_id = id_hash});
        if (error)
            goto cleanup;

        error =
            ecs_world_component_add(world, glyph, CLS_ECS_COMP_UI_LABEL_GROUP,
                                    &(struct ui_label_group){0});
        if (error)
            goto cleanup;

        cursor_x += (float)glyph_data->advance * scale;
    }

    if (label)
        *label = root;

    return CLS_SUCCESS;

cleanup:
    // TODO: Cleanup all entities, would need to make an array to keep track of
    // all characters and delete them later
    // Or use the built in group id component
    return error;
}
