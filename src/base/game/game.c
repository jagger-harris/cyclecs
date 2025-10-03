#include "base/game/game.h"
#include "base/ecs/component/board.h"
#include "base/ecs/component/board_button.h"
#include "base/ecs/system/board_button.h"
#include "base/ecs/system/move.h"
#include "core/app/app.h"
#include "core/app/assets.h"
#include "core/app/component/renderable/mesh.h"
#include "core/app/component/renderable/renderable.h"
#include "core/app/component/renderable/sprite.h"
#include "core/app/ecs.h"
#include "core/app/event_queue.h"
#include "core/app/ui.h"
#include "core/gfx/color.h"
#include "core/gfx/renderer/renderer.h"
#include "core/gfx/texture2d.h"
#include "core/gfx/transform.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/table.h"
#include "core/util/types.h"
#include <cglm/types.h>
#include <cglm/vec2.h>
#include <math.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int fps_event(void *ctx, void *data) {
    struct ui_widget_label *label = ctx;
    float *fps = (float *)data;

    int ret =
        snprintf(label->text, UI_WIDGET_TEXT_MAX, "FPS: %i", (int)roundf(*fps));
    if (ret < 0)
        return CORE_FAILURE;

    return CORE_SUCCESS;
}

int game_init(struct game_state *out, struct app *app) {
    struct assets *assets = &app->assets;
    struct ecs *ecs = &app->ecs;
    struct ui *ui = &app->window.ui;
    struct renderer *renderer = &app->window.renderer;

    // **************** ASSETS ****************
    assets_font_add(assets, "humansans_regular.otf", 64);
    assets_texture2d_add(assets, "xo.png", TEXTURE_FILTER_LINEAR,
                         TEXTURE_WRAP_CLAMP);

    // **************** ECS ****************
    ecs_add_world(ecs, "main_world");

    struct ecs_world *world = NULL;
    ecs_get_world(&world, ecs, "main_world");

    ecs_world_component_type_add(world, "transform", sizeof(struct transform));
    ecs_world_component_type_add(world, "renderable",
                                 sizeof(struct renderable));
    ecs_world_component_type_add(world, "mesh", sizeof(struct mesh));
    ecs_world_component_type_add(world, "sprite", sizeof(struct sprite));
    ecs_world_component_type_add(world, "board", sizeof(struct board));
    ecs_world_component_type_add(world, "board_button",
                                 sizeof(struct board_button));
    ecs_world_system_add(world, "move", move_system);
    ecs_world_system_add(world, "board_button", board_button_system);

    // **************** GAME ****************
    u32 present_bar_entity = game_mesh2d_entity_new(
        world, "quad", "", (vec2){0.0f, 0.0f}, (vec2){400.0f, 999999.0f}, 0.0f,
        -0.5f, (struct color){255, 100, 100, 255});

    *out = (struct game_state){.turn = PLAYER_X,
                               .timelines = 0,
                               .present = 0,
                               .winner = PLAYER_NONE,
                               .present_bar_entity = present_bar_entity};
    struct game_state *state = out;

    renderer_add_camera_ortho(renderer, "main_camera", (vec3){0.0f, 0.0f, 0.0f},
                              0.0f, (float)app->window.size[0],
                              (float)app->window.size[1], 0.0f, 1.0f, -1.0f,
                              1.0f);
    renderer_active_camera_set(renderer, "main_camera");

    enum player board_state[9] = {PLAYER_NONE};
    game_board_entity_new(world, PLAYER_X, (vec2){0.0f, 0.0f}, state->present,
                          state->timelines, board_state);

    ui_widget_add_label(ui, "test_label",
                        "THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG", 64,
                        "humansans_regular.otf", true,
                        (struct transform){.pos = {100.0f, 500.0f, 1.0f},
                                           .scale = {100.0f, 100.0f, 1.0f}},
                        (struct color){255, 255, 255, 255}, NULL, NULL);

    ui_widget_add_label(ui, "fps_label", "FPS: ", 20, "humansans_regular.otf",
                        true,
                        (struct transform){.pos = {10.0f, 25.0f, 1.0f},
                                           .scale = {100.0f, 100.0f, 1.0f}},
                        (struct color){255, 255, 255, 255}, NULL, NULL);

    char fps_widget_id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(fps_widget_id, GLOBALS_STR_ID_MAX, "%s", "fps_label");
    if (ret < 0)
        return CORE_FAILURE;

    struct ui_widget *fps_widget = NULL;
    int error = table_find((void **)&fps_widget, &ui->widgets, fps_widget_id);
    if (error)
        return error;

    struct ui_widget_label *label = NULL;
    error = array_get((void **)&label, &ui->labels, fps_widget->data_index);
    if (error)
        return error;

    event_queue_add_subject(&app->event_queue, "fps_text");
    event_queue_subject_add_observer(&app->event_queue, "fps_text", fps_event,
                                     label);
    return CORE_SUCCESS;
}

int game_update(struct app *app) {
    int error = event_queue_push(&app->event_queue, "fps_text",
                                 &app->window.timing.fps_avg);
    if (error)
        return error;

    return CORE_SUCCESS;
}

u32 game_mesh2d_entity_new(struct ecs_world *world, const char *mesh_id,
                           const char *texture_id, vec2 pos, vec2 scale,
                           float rot_angle, float z_index, struct color tint) {
    struct mesh mesh = {.tint = tint};
    struct renderable ren = {.visible = true, .opacity = 1.0f};
    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .scale = {scale[0], scale[1], 1.0f},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = rot_angle};

    int ret =
        snprintf(mesh.texture_id, sizeof(mesh.texture_id), "%s", texture_id);
    if (ret < 0)
        return U32_MAX;

    ret = snprintf(mesh.mesh_id, sizeof(mesh.mesh_id), "%s", mesh_id);
    if (ret < 0)
        return U32_MAX;

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, "mesh", &mesh);
    ecs_world_component_add(world, new_entity, "renderable", &ren);
    ecs_world_component_add(world, new_entity, "transform", &tf);
    return new_entity;
}

u32 game_sprite_entity_new(struct ecs_world *world, const char *texture_id,
                           vec2 pos, vec2 scale, float rot_angle, float z_index,
                           struct color tint, vec2 uv_offset, vec2 uv_scale,
                           bool visible) {
    struct sprite sprite = {.tint = tint,
                            .uv_offset = {uv_offset[0], uv_offset[1]},
                            .uv_scale = {uv_scale[0], uv_scale[1]}};
    struct renderable ren = {.visible = visible, .opacity = 1.0f};
    struct transform tf = {.pos = {pos[0], pos[1], z_index},
                           .scale = {scale[0], scale[1]},
                           .rot_axis = {0.0f, 0.0f, 1.0f},
                           .rot_angle = rot_angle};

    int ret = snprintf(sprite.texture_id, sizeof(sprite.texture_id), "%s",
                       texture_id);
    if (ret < 0)
        return U32_MAX;

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, "sprite", &sprite);
    ecs_world_component_add(world, new_entity, "renderable", &ren);
    ecs_world_component_add(world, new_entity, "transform", &tf);
    return new_entity;
}

u32 game_board_button_entity_new(struct ecs_world *world, u32 board_entity,
                                 size_t id, vec2 pos, vec2 scale,
                                 enum player turn) {
    bool visible = false;
    struct color color = {0};
    vec2 uv_offset = {0};
    if (turn == PLAYER_X || turn == PLAYER_O)
        visible = true;

    switch (turn) {
    case PLAYER_X:
        visible = true;
        color = (struct color){255, 100, 100, 255};
        glm_vec2_copy((vec2){0.0f, 0.0f}, uv_offset);
        break;
    case PLAYER_O:
        visible = true;
        color = (struct color){100, 100, 255, 255};
        glm_vec2_copy((vec2){0.5f, 0.0f}, uv_offset);
        break;
    case PLAYER_NONE:
    default:
        break;
    }

    u32 background_entity =
        game_mesh2d_entity_new(world, "quad", "", pos, scale, 0.0f, 0.5f,
                               (struct color){0, 0, 0, 255});
    u32 new_entity =
        game_sprite_entity_new(world, "xo.png", pos, scale, 0.0f, 0.75f, color,
                               uv_offset, (vec2){0.5f, 0.5f}, visible);
    struct board_button button = {.id = id,
                                  .turn = turn,
                                  .background_entity = background_entity,
                                  .board_entity = board_entity};

    ecs_world_component_add(world, new_entity, "board_button", &button);
    return new_entity;
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

    struct color bg_tint = owner == PLAYER_X
                               ? (struct color){255, 100, 100, 255}
                               : (struct color){100, 100, 255, 255};

    u32 background = game_mesh2d_entity_new(
        world, "quad", "", pos, (vec2){board_border_size, board_border_size},
        0.0f, 0.0f, bg_tint);

    float board_background_size =
        gap * ((float)rows - 1.0f) + (button_size * (float)rows);

    game_mesh2d_entity_new(world, "quad", "", pos,
                           (vec2){board_background_size, board_background_size},
                           0.0f, 0.25f, (struct color){255, 255, 255, 255});

    struct board board = {.owner = owner,
                          .background_entity = background,
                          .present = present,
                          .timeline = timeline,
                          .has_next_board = false};
    memcpy(board.state, state, sizeof(board.state));

    u32 new_entity = U32_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, "board", &board);

    for (int i = 0; i < rows * rows; ++i) {
        int col = i % rows;
        int row = i / rows;

        float x = pos[0] + start_x + (float)col * spacing;
        float y = pos[1] + start_y - (float)row * spacing;

        game_board_button_entity_new(world, new_entity, i, (vec2){x, y},
                                     (vec2){button_size, button_size},
                                     board.state[i]);
    }

    return new_entity;
}
