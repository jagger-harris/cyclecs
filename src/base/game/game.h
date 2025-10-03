#ifndef GAME_H
#define GAME_H

#include "base/ecs/component/board.h"
#include "cglm/types.h"
#include <stddef.h>

struct arena;
struct app;
struct color;
struct ecs_world;

struct game_state {
    enum player turn;
    enum player winner;
    u32 present_bar_entity;
    unsigned int present;
    unsigned int timelines;
};

int game_init(struct game_state *out, struct app *app);
int game_update(struct app *app);
u32 game_mesh2d_entity_new(struct ecs_world *world, const char *mesh_path,
                           const char *texture_path, vec2 pos, vec2 scale,
                           float rot_angle, float z_index, struct color tint);
u32 game_sprite_entity_new(struct ecs_world *world, const char *texture_id,
                           vec2 pos, vec2 scale, float rot_angle, float z_index,
                           struct color tint, vec2 uv_offset, vec2 uv_scale,
                           bool visible);
u32 game_board_button_entity_new(struct ecs_world *world, u32 board_entity,
                                 size_t id, vec2 pos, vec2 scale,
                                 enum player turn);
u32 game_board_entity_new(struct ecs_world *world, enum player owner, vec2 pos,
                          unsigned int present, unsigned int timeline,
                          enum player state[9]);
u32 game_camera_orthographic_entity_new(struct ecs_world *world, float zoom);

#endif // GAME_H
