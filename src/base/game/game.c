#include "base/game/game.h"
#include "base/ecs/system/move.h"
#include "core/ecs/component/renderable/mesh2d.h"
#include "core/ecs/component/renderable/renderable2d.h"
#include "core/ecs/component/renderable/sprite2d.h"
#include "core/ecs/ecs.h"
#include "core/ecs/world.h"
#include "core/util/logger.h"
#include <stdint.h>

static struct vertex quad_vertices[] = {
    {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
    {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
    {{0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
    {{-0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}}};
static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};
static size_t vertex_count = ARRAY_SIZE(quad_vertices);
static size_t index_count = ARRAY_SIZE(quad_indices);

static u64 sprite2d_entity_new(struct ecs_world *world, float pos_x,
                               float pos_y, float scale_x, float scale_y,
                               float rot_angle, float z_index) {
    struct sprite2d board_sprite = {.texture_path = "container.jpg"};
    struct renderable2d board_ren = {.visible = true, .opacity = 1.0F};
    struct transform2d board_tf = {.pos = {pos_x, pos_y},
                                   .scale = {scale_x, scale_y},
                                   .rot_angle = rot_angle,
                                   .z_index = z_index};

    u64 new_entity = UINT64_MAX;
    ecs_world_entity_add(&new_entity, world);
    ecs_world_component_add(world, new_entity, "sprite2d", &board_sprite);
    ecs_world_component_add(world, new_entity, "renderable2d", &board_ren);
    ecs_world_component_add(world, new_entity, "transform2d", &board_tf);
    return new_entity;
}

int game_init(struct game *out, struct app *app) {
    struct assets *assets = &app->assets;
    struct ecs *ecs = &app->ecs;

    // **************** ASSETS ****************
    assets_material_add(assets, "container", "sprite2d", "container.jpg");
    assets_mesh_add(assets, "sprite2d", quad_vertices, vertex_count,
                    quad_indices, index_count);

    // **************** ECS ****************
    ecs_add_world(ecs, "world");

    struct ecs_world *world = NULL;
    ecs_get_world(&world, ecs, "world");

    ecs_world_component_type_add(world, "transform2d",
                                 sizeof(struct transform2d));
    ecs_world_component_type_add(world, "renderable2d",
                                 sizeof(struct renderable2d));
    ecs_world_component_type_add(world, "mesh2d", sizeof(struct mesh2d));
    ecs_world_component_type_add(world, "sprite2d", sizeof(struct sprite2d));
    ecs_world_system_add(world, "move", move_sys);

    // **************** GAME ****************
    sprite2d_entity_new(world, 0.0F, 0.0F, 1.0F, 1.0F, 10.0F, 1);
    sprite2d_entity_new(world, -0.5F, 0.75F, 0.25F, 1.0F, 10.0F, 1);
    out->timelines = 0;
    out->present = 0;

    return CORE_SUCCESS;
}

int game_run(struct app *app) {
    struct renderer *renderer = &app->window.renderer;
    struct input *input = &app->window.input;

    if (input->keys[GLFW_KEY_W])
        renderer_camera_move(renderer, 0.0F, 0.01F, 0.0F);

    if (input->keys[GLFW_KEY_S])
        renderer_camera_move(renderer, 0.0F, -0.01F, 0.0F);

    if (input->keys[GLFW_KEY_A])
        renderer_camera_move(renderer, 0.01F, 0.0F, 0.0F);

    if (input->keys[GLFW_KEY_D])
        renderer_camera_move(renderer, -0.01F, 0.0F, 0.0F);

    if (input->mouse_buttons[GLFW_MOUSE_BUTTON_LEFT]) {
        LOGGER_LOG(LOGGER_DEBUG, "%f %f", input->cursor_pos[0],
                   input->cursor_pos[1]);
    }

    return CORE_SUCCESS;
}
