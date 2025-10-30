#ifndef GAME_H
#define GAME_H

#include "base/ecs/component/board.h"
#include "core/ecs/world.h"
#include <cglm/types.h>
#include <stddef.h>

struct arena;
struct app;
struct assets;
struct ecs_world;

struct game_state {
    enum player turn;
    enum player winner;
    u32 present_bar_entity;
    unsigned int present;
    unsigned int timelines;
};

enum game_ecs_worlds { GAME_WORLD_MAIN = 0, GAME_WORLD_UI };
enum game_comp_types {
    GAME_COMP_BOARD = ECS_COMP_LENGTH,
    GAME_COMP_BOARD_BUTTON,
    GAME_COMP_ARROW
};
enum game_sys {
    GAME_SYS_CAMERA = ECS_SYS_LENGTH,
    GAME_SYS_UI_BUTTON,
    GAME_SYS_BOARD_BUTTON,
    GAME_SYS_WINNER,
    GAME_SYS_BENCHMARK
};

int game_init(struct game_state *out, struct app *app);
int game_update(struct app *app);
u32 game_ui_label_entity_new(struct ecs_world *world, struct assets *assets,
                             const char *id, vec2 pos, float z_index,
                             const char *text, int font_size,
                             const char *font_id, bool visible, ivec4 tint);
void game_ui_label_entity_delete(struct ecs_world *world, u32 entity);
u32 game_camera_ortho_entity_new(struct ecs_world *world, vec3 pos, float left,
                                 float right, float bottom, float top,
                                 float zoom, float near_clip, float far_clip,
                                 bool y_down, bool active);
u32 game_renderable_entity_new(struct ecs_world *world, const char *mesh_path,
                               const char *shader_id, const char *texture_path,
                               vec3 pos, vec3 scale, float rot_angle,
                               vec2 uv_offset, vec2 uv_scale, ivec4 tint,
                               bool visible, bool transparent);
u32 game_rect_entity_new(struct ecs_world *world, const char *texture2d_id,
                         vec2 pos, vec2 scale, float rot_angle, float z_index,
                         ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                         bool visible);
u32 game_sprite_entity_new(struct ecs_world *world, const char *texture_id,
                           vec2 pos, vec2 scale, float rot_angle, float z_index,
                           ivec4 tint, vec2 uv_offset, vec2 uv_scale,
                           bool visible);
u32 game_board_button_entity_new(struct ecs_world *world, u32 board_entity,
                                 size_t id, vec2 pos, vec2 scale,
                                 enum player turn);
u32 game_board_entity_new(struct ecs_world *world, enum player owner, vec2 pos,
                          unsigned int present, unsigned int timeline,
                          enum player state[9]);
u32 game_arrow_entity_new(struct ecs_world *world, bool time_traveled,
                          vec2 board_from_pos, vec2 board_to_pos,
                          unsigned int timeline_from, unsigned int timeline_to);

#endif // GAME_H
