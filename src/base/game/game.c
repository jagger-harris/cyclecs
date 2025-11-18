#include "base/game/game.h"
#include "base/ecs/system/main/board_button.h"
#include "base/ecs/system/main/camera.h"
#include "base/ecs/system/main/winner.h"
#include "base/ecs/system/ui/benchmark.h"
#include "core/app/assets.h"
#include "core/app/window.h"
#include "core/ecs/component/node.h"
#include "core/ecs/component/renderable/renderable.h"
#include "core/ecs/component/renderable/ui.h"
#include "core/ecs/component/transform.h"
#include "core/ecs/ecs.h"
#include "core/io/ffont.h"
#include "core/util/array.h"
#include "core/util/xxhash32.h"

static int add_component_types(struct ecs_world *world, void *data) {
    (void)data;

    ecs_world_component_type_add(world, ECS_COMP_NODE, sizeof(struct node));
    ecs_world_component_type_add(world, ECS_COMP_TRANSFORM,
                                 sizeof(struct transform));
    ecs_world_component_type_add(world, ECS_COMP_RENDERABLE,
                                 sizeof(struct renderable));
    ecs_world_component_type_add(world, ECS_COMP_CAMERA, sizeof(struct camera));
    ecs_world_component_type_add(world, ECS_COMP_UI_BASE,
                                 sizeof(struct ui_base));
    ecs_world_component_type_add(world, ECS_COMP_UI_BUTTON,
                                 sizeof(struct ui_button));
    ecs_world_component_type_add(world, ECS_COMP_UI_LABEL,
                                 sizeof(struct ui_label));
    return CORE_SUCCESS;
}

int game_init(struct game_state *out, struct app *app) {
    struct assets *assets = app->assets;
    struct ecs *ecs = app->ecs;

    // **************** ASSETS ****************
    assets_texture2d_add(assets, "xo.png", TEXTURE_FILTER_LINEAR,
                         TEXTURE_WRAP_CLAMP);

    // **************** ECS ****************
    ecs_world_add(ecs, GAME_WORLD_MAIN, 0.0f, 0, true);
    ecs_world_add(ecs, GAME_WORLD_UI, 20.0f, 0, true);

    ecs_iter_all_worlds(ecs, add_component_types, NULL);

    struct ecs_world *main_world = NULL;
    ecs_world_get(&main_world, ecs, GAME_WORLD_MAIN);

    ecs_world_component_type_add(main_world, GAME_COMP_BOARD,
                                 sizeof(struct board));
    ecs_world_component_type_add(main_world, GAME_COMP_BOARD_BUTTON,
                                 sizeof(struct board_button));

    ecs_world_system_add(main_world, GAME_SYS_CAMERA, main_camera_system, 1,
                         ECS_COMP_CAMERA);
    ecs_world_system_add(main_world, ECS_SYS_UI_BUTTON, ui_button_system, 3,
                         ECS_COMP_UI_BASE, ECS_COMP_UI_BUTTON,
                         ECS_COMP_TRANSFORM);
    ecs_world_system_add(main_world, GAME_SYS_BOARD_BUTTON,
                         main_board_button_system, 3, ECS_COMP_UI_BUTTON,
                         GAME_COMP_BOARD_BUTTON, ECS_COMP_NODE);
    ecs_world_system_add(main_world, GAME_SYS_WINNER, main_winner_system, 1,
                         GAME_COMP_BOARD);

    struct ecs_world *ui_world = NULL;
    ecs_world_get(&ui_world, ecs, GAME_WORLD_UI);

    ecs_world_system_add(ui_world, GAME_SYS_BENCHMARK, ui_benchmark_system, 2,
                         ECS_COMP_UI_BASE, ECS_COMP_UI_LABEL);

    // **************** GAME ****************

    u32 present_bar_entity = game_rect_entity_new(
        main_world, "", (vec2){0.0f, 0.0f}, (vec2){400.0f, 999999.0f}, 0.0f,
        -0.5f, (ivec4){255, 100, 100, 255}, (vec2){1.0f, 1.0f},
        (vec2){1.0f, 1.0f}, true);

    *out = (struct game_state){.turn = PLAYER_X,
                               .timelines = 0,
                               .present = 0,
                               .winner = PLAYER_NULL,
                               .present_bar_entity = present_bar_entity};
    struct game_state *state = out;

    ivec2 fb_size = {0};
    int error = window_fb_size_get(fb_size, app->window);
    if (error)
        return error;

    game_camera_ortho_entity_new(main_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f,
                                 (float)fb_size[0], (float)fb_size[1], 0.0f,
                                 1.0f, -1.0f, 1.0f, true, true);

    for (int i = 0; i < 3500; ++i) {
        enum player board_state[9] = {PLAYER_NULL};
        game_board_entity_new(main_world, PLAYER_X, (vec2){0.0f, 0.0f},
                              state->present, state->timelines, board_state);
    }

    game_camera_ortho_entity_new(ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f,
                                 (float)fb_size[0], (float)fb_size[1], 0.0f,
                                 1.0f, -1.0f, 1.0f, true, true);

    game_ui_label_entity_new(
        ui_world, assets, "winner_text", (vec2){100.0f, 500.0f}, 1.0f, "Tie!",
        150, "human_sans-regular.otf", true, (ivec4){255, 255, 255, 255});

    game_ui_label_entity_new(ui_world, assets, "fps", (vec2){10.0f, 25.0f},
                             1.0f, "", 20, "human_sans-regular.otf", true,
                             (ivec4){255, 255, 255, 255});

    game_ui_label_entity_new(ui_world, assets, "entities", (vec2){10.0f, 45.0f},
                             1.0f, "", 20, "human_sans-regular.otf", true,
                             (ivec4){255, 255, 255, 255});

    return CORE_SUCCESS;
}

int game_update(struct app *app) {
    (void)app;
    return CORE_SUCCESS;
}

u32 game_camera_ortho_entity_new(struct ecs_world *world, vec3 pos, float left,
                                 float right, float bottom, float top,
                                 float zoom, float near_clip, float far_clip,
                                 bool y_down, bool active) {
    struct camera camera = {.type = CAMERA_ORTHO,
                            .ortho.left = left,
                            .ortho.right = right,
                            .ortho.bottom = bottom,
                            .ortho.top = top,
                            .ortho.y_down = y_down,
                            .zoom = zoom,
                            .near_clip = near_clip,
                            .far_clip = far_clip,
                            .active = active,
                            .update = true};
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]}};

    glm_mat4_identity(camera.view);
    glm_mat4_identity(camera.projection);

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, ECS_COMP_CAMERA, &camera);
    ecs_world_component_add(world, new_entity, ECS_COMP_TRANSFORM, &tf);
    return new_entity;
}

u32 game_renderable_entity_new(struct ecs_world *world, const char *mesh_id,
                               const char *shader_id, const char *texture2d_id,
                               vec3 pos, vec3 scale, float rot_angle,
                               vec2 uv_offset, vec2 uv_scale, ivec4 tint,
                               bool visible, bool transparent) {
    struct renderable ren = {.uv_offset = {uv_offset[0], uv_offset[1]},
                             .uv_scale = {uv_scale[0], uv_scale[1]},
                             .opacity = 1.0f,
                             .visible = visible,
                             .transparent = transparent};
    struct transform tf = {.pos = {pos[0], pos[1], pos[2]},
                           .scale = {scale[0], scale[1], scale[2]},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = rot_angle};

    glm_ivec4_copy(tint, ren.tint);

    int error = xxhash32(&ren.mesh_id, mesh_id, strlen(mesh_id), 0);
    if (error)
        return U32_MAX;

    error = xxhash32(&ren.shader_id, shader_id, strlen(shader_id), 0);
    if (error)
        return U32_MAX;

    error = xxhash32(&ren.texture_id, texture2d_id, strlen(texture2d_id), 0);
    if (error)
        return U32_MAX;

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, ECS_COMP_RENDERABLE, &ren);
    ecs_world_component_add(world, new_entity, ECS_COMP_TRANSFORM, &tf);
    return new_entity;
}

u32 game_rect_entity_new(struct ecs_world *world, const char *texture2d_id,
                         vec2 pos, vec2 scale, float rot_angle, float z_index,
                         ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                         bool visible) {
    return game_renderable_entity_new(
        world, "quad", "mesh", texture2d_id, (vec3){pos[0], pos[1], z_index},
        (vec3){scale[0], scale[1], 1.0f}, rot_angle, uv_offset, uv_scale, tint,
        visible, false);
}

u32 game_sprite_entity_new(struct ecs_world *world, const char *texture2d_id,
                           vec2 pos, vec2 scale, float rot_angle, float z_index,
                           ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                           bool visible) {
    return game_renderable_entity_new(
        world, "quad", "sprite", texture2d_id, (vec3){pos[0], pos[1], z_index},
        (vec3){scale[0], scale[1], 1.0f}, rot_angle, uv_offset, uv_scale, tint,
        visible, true);
}

u32 game_ui_image_button_entity_new(struct ecs_world *world, const char *id,
                                    const char *image_id, vec2 pos,
                                    float z_index, vec2 scale, vec2 uv_offset,
                                    vec2 uv_scale, ivec4 image_tint,
                                    bool visible) {
    struct node node = {.parent = U32_MAX, .children = NULL};
    struct ui_base base = {.interactable = true};
    struct ui_button button = {0};
    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .scale = {scale[0], scale[1], 1.0f},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = 0.0f};

    int ret = snprintf(base.id, sizeof(base.id), "%s", id);
    if (ret < 0)
        return U32_MAX;

    u32 new_entity = U32_MAX;
    int error = ecs_world_entity_add(&new_entity, world);
    if (error)
        return U32_MAX;

    error = ecs_world_component_add(world, new_entity, ECS_COMP_NODE, &node);
    if (error)
        return U32_MAX;

    error = ecs_world_component_add(world, new_entity, ECS_COMP_UI_BASE, &base);
    if (error)
        return U32_MAX;

    error =
        ecs_world_component_add(world, new_entity, ECS_COMP_UI_BUTTON, &button);
    if (error)
        return U32_MAX;

    error = ecs_world_component_add(world, new_entity, ECS_COMP_TRANSFORM, &tf);
    if (error)
        return U32_MAX;

    u32 background_entity = game_rect_entity_new(world, "", pos, scale, 0.0f,
                                                 z_index, (ivec4){0, 0, 0, 255},
                                                 uv_offset, uv_scale, visible);
    if (background_entity == U32_MAX)
        return U32_MAX;

    u32 image_entity = game_sprite_entity_new(world, image_id, pos, scale, 0.0f,
                                              z_index + 0.01f, image_tint,
                                              uv_offset, uv_scale, visible);
    if (image_entity == U32_MAX)
        return U32_MAX;

    struct node *node_data = NULL;
    error = ecs_world_query_get_single((void **)&node_data, world, new_entity,
                                       ECS_COMP_NODE);
    if (error || !node_data)
        goto cleanup;

    error = array_create(&node_data->children, 2, sizeof(u32));
    if (error)
        goto cleanup;

    error = array_push(&node_data->children, &background_entity);
    if (error)
        goto cleanup;

    error = array_push(&node_data->children, &image_entity);
    if (error)
        goto cleanup;

    return new_entity;

cleanup:
    if (node_data)
        array_destroy(node_data->children);

    return U32_MAX;
}

u32 game_ui_label_entity_new(struct ecs_world *world, struct assets *assets,
                             const char *id, vec2 pos, float z_index,
                             const char *text, int font_size,
                             const char *font_id, bool visible, ivec4 tint) {
    struct node node = {.parent = U32_MAX, .children = NULL};
    struct ui_base base = {.interactable = false};
    struct ui_label label = {.font_size = font_size};
    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = 0.0f};

    int ret = snprintf(base.id, sizeof(base.id), "%s", id);
    if (ret < 0)
        return U32_MAX;

    ret = snprintf(label.text, sizeof(label.text), "%s", text);
    if (ret < 0)
        return U32_MAX;

    int error = xxhash32(&label.font_id, font_id, strlen(font_id), 0);
    if (error)
        return U32_MAX;

    struct ffont *font = NULL;
    error = assets_font_get(&font, assets, label.font_id);
    if (error)
        return U32_MAX;

    if (!font) {
        LOGGER_LOG(LOGGER_ERROR, "%s", "No font found for rendering ui label");
        return U32_MAX;
    }

    u32 new_entity = U32_MAX;
    error = ecs_world_entity_add(&new_entity, world);
    if (error)
        return U32_MAX;

    error = ecs_world_component_add(world, new_entity, ECS_COMP_NODE, &node);
    if (error)
        return U32_MAX;

    error = ecs_world_component_add(world, new_entity, ECS_COMP_UI_BASE, &base);
    if (error)
        return U32_MAX;

    error =
        ecs_world_component_add(world, new_entity, ECS_COMP_UI_LABEL, &label);
    if (error)
        return U32_MAX;

    struct node *node_data = NULL;
    error = ecs_world_query_get_single((void **)&node_data, world, new_entity,
                                       ECS_COMP_NODE);
    if (error)
        goto cleanup;

    error = array_create(&node_data->children, 8, sizeof(u32));
    if (error)
        goto cleanup;

    float scale = (float)label.font_size / (float)font->pixel_size;
    float cursor_x = tf.pos[0];
    float cursor_y = tf.pos[1];
    size_t text_length = strlen(label.text);

    for (size_t i = 0; i < text_length; ++i) {
        unsigned char c = (unsigned char)label.text[i];
        if (c < FFONT_CHAR_START || c > FFONT_CHAR_END)
            continue;

        struct fglyph *glyph = &font->glyphs[c - FFONT_CHAR_START];
        if (glyph->width == 0 || glyph->height == 0) {
            cursor_x += (float)glyph->advance * scale;
            continue;
        }

        float pos_x = cursor_x + (float)glyph->bearing_x * scale +
                      ((float)glyph->width * scale * 0.5f);
        float pos_y = cursor_y - (float)glyph->bearing_y * scale +
                      ((float)glyph->height * scale * 0.5f);

        vec3 glyph_pos = {pos_x, pos_y, tf.pos[2]};
        vec3 glyph_scale = {(float)glyph->width * scale,
                            (float)glyph->height * scale, 1.0f};

        u32 text_entity = game_renderable_entity_new(
            world, "quad", "font", font_id, glyph_pos, glyph_scale, 0.0f,
            (vec2){(float)glyph->atlas_x / (float)font->atlas_width,
                   (float)glyph->atlas_y / (float)font->atlas_height},
            (vec2){(float)glyph->width / (float)font->atlas_width,
                   (float)glyph->height / (float)font->atlas_height},
            tint, visible, true);
        if (text_entity == U32_MAX)
            goto cleanup;

        error = array_push(&node_data->children, &text_entity);
        if (error)
            goto cleanup;

        cursor_x += (float)glyph->advance * scale;
    }

    return new_entity;

cleanup:
    game_ui_label_entity_delete(world, new_entity);
    return U32_MAX;
}

// TODO: Can have a universal delete functiion inside ecs with a node component
// eventually
void game_ui_label_entity_delete(struct ecs_world *world, u32 entity) {
    if (!world)
        return;

    if (entity == U32_MAX)
        return;

    struct node *node_data = NULL;
    int error = ecs_world_query_get_single((void **)&node_data, world, entity,
                                           ECS_COMP_NODE);
    if (error)
        return;

    if (!node_data)
        return;

    size_t children_length = 0;
    error = array_length_get(&children_length, node_data->children);
    if (error)
        return;

    u32 child_entity = U32_MAX;
    for (size_t i = 0; i < children_length; ++i) {
        error = array_elem_get_cpy(&child_entity, node_data->children, i);
        if (error)
            continue;

        error = ecs_world_entity_remove(world, child_entity);
        if (error)
            LOGGER_LOG(LOGGER_ERROR, "%s",
                       "Removing label child entity failed");
    }

    array_destroy(node_data->children);
    ecs_world_entity_remove(world, entity);
}

u32 game_board_entity_new(struct ecs_world *world, enum player owner, vec2 pos,
                          unsigned int present, unsigned int timeline,
                          enum player state[9]) {
    int rows = 3;
    float button_size = 200.0f;
    float gap = 6.0f;
    float spacing = button_size + gap;

    float grid_width = ((float)rows - 1.0f) * spacing;
    float grid_height = ((float)rows - 1.0f) * spacing;

    float start_x = -grid_width * 0.5f;
    float start_y = grid_height * 0.5f;

    float board_border_size =
        gap * ((float)rows + 1.0f) + (button_size * (float)rows);

    ivec4 bg_tint = {255, 100, 100, 255};

    if (owner == PLAYER_O)
        glm_ivec4_copy((ivec4){100, 100, 255, 255}, bg_tint);

    u32 background = game_rect_entity_new(
        world, "", pos, (vec2){board_border_size, board_border_size}, 0.0f,
        0.0f, bg_tint, (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);

    float board_background_size =
        gap * ((float)rows - 1.0f) + (button_size * (float)rows);

    game_rect_entity_new(world, "", pos,
                         (vec2){board_background_size, board_background_size},
                         0.0f, 0.25f, (ivec4){255, 255, 255, 255},
                         (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);

    struct board board = {.owner = owner,
                          .background_entity = background,
                          .present = present,
                          .timeline = timeline,
                          .has_next_board = false};
    memcpy(board.state, state, sizeof(board.state));

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, GAME_COMP_BOARD, &board);

    for (int i = 0; i < rows * rows; ++i) {
        int col = i % rows;
        int row = i / rows;

        float x = pos[0] + start_x + (float)col * spacing;
        float y = pos[1] + start_y - (float)row * spacing;

        vec2 uv_offset = {0};
        ivec4 image_tint = {0};

        if (board.state[i] == PLAYER_X) {
            glm_ivec4_copy((ivec4){255, 100, 100, 255}, image_tint);
        } else if (board.state[i] == PLAYER_O) {
            glm_vec2_copy((vec2){0.5f, 0.0f}, uv_offset);
            glm_ivec4_copy((ivec4){100, 100, 255, 255}, image_tint);
        }

        u32 new_button_entity = game_ui_image_button_entity_new(
            world, "board_button", "xo.png", (vec2){x, y}, 0.5f,
            (vec2){button_size, button_size}, uv_offset, (vec2){0.5f, 0.5f},
            image_tint, true);

        ecs_world_component_add(
            world, new_button_entity, GAME_COMP_BOARD_BUTTON,
            &(struct board_button){.board_entity = new_entity,
                                   .id = (size_t)i,
                                   .turn = board.state[i]});
    }

    return new_entity;
}
