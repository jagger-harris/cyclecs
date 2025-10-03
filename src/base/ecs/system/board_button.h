#ifndef ECS_SYSTEM_BOARD_BUTTON_H
#define ECS_SYSTEM_BOARD_BUTTON_H

#include "base/ecs/component/board.h"
#include "base/ecs/component/board_button.h"
#include "base/game/game.h"
#include "core/app/app.h"
#include "core/app/component/renderable/mesh.h"
#include "core/app/component/renderable/renderable.h"
#include "core/app/component/renderable/sprite.h"
#include "core/app/ecs.h"
#include "core/app/input.h"
#include "core/app/window.h"
#include "core/gfx/renderer/renderer.h"
#include "core/gfx/transform.h"
#include "core/util/array.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <string.h>
#include <time.h>

struct ecs_world;

static enum player check_winner(enum player state[9]) {
    static const int wins[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Cols
        {0, 4, 8}, {2, 4, 6} // Diagonals
    };

    for (int i = 0; i < 8; ++i) {
        int a = wins[i][0], b = wins[i][1], c = wins[i][2];
        if (state[a] != PLAYER_NONE && state[a] == state[b] &&
            state[a] == state[c]) {
            return state[a];
        }
    }

    return PLAYER_NONE;
}

int board_button_system(struct ecs_world *in, struct app *app) {
    if (!in)
        return CORE_NULLPTR;

    u64 entity_current = U64_MAX;
    struct game_state *state = app->game_state;
    struct array *board_button_entities = NULL;
    struct transform *transform_current = NULL;
    struct renderable *renderable_current = NULL;
    struct mesh *mesh_current = NULL;
    struct sprite *sprite_current = NULL;
    struct board *board_current = NULL;
    struct board_button *board_button_current = NULL;

    ecs_world_query_all_entities(&board_button_entities, in, "board_button");

    for (size_t i = 0; i < board_button_entities->length; ++i) {
        array_get_cpy(&entity_current, board_button_entities, i);
        int error = ecs_world_query_data((void **)&board_button_current, in,
                                         entity_current, "board_button");
        if (error)
            continue;

        error = ecs_world_query_data((void **)&transform_current, in,
                                     entity_current, "transform");
        if (error)
            continue;

        error = ecs_world_query_data((void **)&sprite_current, in,
                                     entity_current, "sprite");
        if (error)
            continue;

        float half_width = transform_current->scale[0] * 0.5f;
        float half_height = transform_current->scale[1] * 0.5f;

        float min_x = transform_current->pos[0] - half_width;
        float max_x = transform_current->pos[0] + half_width;
        float min_y = transform_current->pos[1] - half_height;
        float max_y = transform_current->pos[1] + half_height;

        vec2 cursor_world = {0};
        renderer_screen_to_world(cursor_world, &app->window.renderer,
                                 app->window.input.cursor_pos,
                                 app->window.size);

        bool inside = cursor_world[0] >= min_x && cursor_world[0] <= max_x &&
                      cursor_world[1] >= min_y && cursor_world[1] <= max_y;

        error = ecs_world_query_data((void **)&mesh_current, in,
                                     board_button_current->background_entity,
                                     "mesh");
        if (error)
            continue;

        board_button_current->hovered = inside;

        mesh_current->tint = board_button_current->hovered
                                 ? (struct color){50, 50, 50, 255}
                                 : (struct color){0, 0, 0, 255};

        error =
            ecs_world_query_data((void **)&board_current, in,
                                 board_button_current->board_entity, "board");
        if (error)
            continue;

        bool time_traveled =
            state->present > board_current->present ? true : false;
        bool game_winner = state->winner;
        bool no_turn_taken = board_button_current->turn == PLAYER_NONE;
        bool not_from_future = board_current->present <= state->present;
        bool owned_by_player = state->turn == board_current->owner;
        bool correct_time_flow =
            (time_traveled && board_current->has_next_board) ||
            (!time_traveled && !board_current->has_next_board);

        bool can_press = !game_winner && no_turn_taken && not_from_future &&
                         owned_by_player && correct_time_flow;

        input_mouse_pressed(&board_button_current->pressed, &app->window.input,
                            GLFW_MOUSE_BUTTON_LEFT);
        input_mouse_released(&board_button_current->released,
                             &app->window.input, GLFW_MOUSE_BUTTON_LEFT);
        input_mouse_down(&board_button_current->down, &app->window.input,
                         GLFW_MOUSE_BUTTON_LEFT);

        if (board_button_current->released && board_button_current->hovered &&
            can_press) {
            error = ecs_world_query_data((void **)&transform_current, in,
                                         board_current->background_entity,
                                         "transform");
            if (error)
                continue;

            float x = transform_current->pos[0];
            float y = transform_current->pos[1];

            enum player new_board_state[9] = {PLAYER_NONE};
            memcpy(new_board_state, board_current->state,
                   sizeof(new_board_state));

            if (time_traveled) {
                state->present = board_current->present + 1;
                state->timelines += 1;
                y = (float)state->timelines * (624 + (624 * 0.5f));
            } else {
                board_current->has_next_board = true;
                state->present += 1;
                board_current->state[board_button_current->id] =
                    board_current->owner;
                board_button_current->turn = board_current->owner;

                switch (board_current->owner) {
                case PLAYER_X:
                    sprite_current->tint = (struct color){255, 100, 100, 255};
                    glm_vec2_copy((vec2){0.0f, 0.0f},
                                  sprite_current->uv_offset);
                    break;
                case PLAYER_O:
                    sprite_current->tint = (struct color){100, 100, 255, 255};
                    glm_vec2_copy((vec2){0.5f, 0.0f},
                                  sprite_current->uv_offset);

                    break;
                case PLAYER_NONE:
                default:
                    LOGGER_LOG(LOGGER_ERROR, "%s",
                               "Impossible board state reached");
                    break;
                }
            }

            error = ecs_world_query_data((void **)&renderable_current, in,
                                         entity_current, "renderable");
            if (error)
                continue;

            if (!time_traveled)
                renderable_current->visible = true;

            enum player winner = check_winner(board_current->state);

            if (winner != PLAYER_NONE) {
                state->winner = winner;
                return CORE_SUCCESS;
            }

            new_board_state[board_button_current->id] = board_current->owner;

            enum player new_board_owner =
                board_current->owner == PLAYER_X ? PLAYER_O : PLAYER_X;
            state->turn = state->turn == PLAYER_X ? PLAYER_O : PLAYER_X;

            game_board_entity_new(
                in, new_board_owner, (vec2){(float)x + 624 + (624 * 0.5f), y},
                state->present, state->timelines, new_board_state);

            error =
                ecs_world_query_data((void **)&transform_current, in,
                                     state->present_bar_entity, "transform");

            error = ecs_world_query_data((void **)&mesh_current, in,
                                         state->present_bar_entity, "mesh");

            struct color present_bar_color =
                state->turn == PLAYER_X ? (struct color){255, 100, 100, 255}
                                        : (struct color){100, 100, 255, 255};

            transform_current->pos[0] = x + 624 + (624 * 0.5f);
            mesh_current->tint = present_bar_color;
        }
    }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_BOARD_BUTTON_H
