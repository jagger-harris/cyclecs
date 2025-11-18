#ifndef ECS_SYSTEM_MAIN_BOARD_BUTTON_H
#define ECS_SYSTEM_MAIN_BOARD_BUTTON_H

#include "base/ecs/component/board_button.h"
#include "base/game/game.h"
#include "core/ecs/component/node.h"
#include "core/ecs/component/renderable/renderable.h"
#include "core/ecs/system/ui_button.h"
#include "core/util/array.h"
#include "core/util/logger.h"

int main_board_button_system(struct ecs_world_query *query, struct app *app) {
    if (!query || !query->world || !app)
        return CORE_NULLPTR;

    struct game_state *game_state = app->game_state;
    bool game_over = game_state->winner != PLAYER_NULL;

    u32 entity;
    while (ecs_world_query_next(&entity, query) == CORE_SUCCESS &&
           entity != U32_MAX) {
        struct ui_button *button = NULL;
        struct board_button *board_button = NULL;
        struct node *node_data = NULL;

        int error = ecs_world_query_get((void **)&button, query,
                                        ECS_COMP_UI_BUTTON, entity);
        if (error)
            continue;

        error = ecs_world_query_get((void **)&board_button, query,
                                    GAME_COMP_BOARD_BUTTON, entity);
        if (error)
            continue;

        error = ecs_world_query_get((void **)&node_data, query, ECS_COMP_NODE,
                                    entity);
        if (error)
            continue;

        if (!button || !board_button || !node_data)
            continue;

        u32 bg_entity = UINT32_MAX;
        error = array_elem_get_cpy(&bg_entity, node_data->children, 0);
        if (error)
            continue;

        struct renderable *bg_ren = NULL;
        error = ecs_world_query_get_single((void **)&bg_ren, query->world,
                                           bg_entity, ECS_COMP_RENDERABLE);
        if (error || !bg_ren)
            continue;

        if (button->hovering) {
            glm_ivec4_copy((ivec4){50, 50, 50, 255}, bg_ren->tint);
        } else {
            glm_ivec4_copy((ivec4){0, 0, 0, 255}, bg_ren->tint);
        }

        if (game_over || !button->released || !button->hovering)
            continue;

        struct board *board = NULL;
        error = ecs_world_query_get_single((void **)&board, query->world,
                                           board_button->board_entity,
                                           GAME_COMP_BOARD);
        if (error || !board)
            continue;

        bool time_traveled = game_state->present > board->present;
        bool no_turn_taken = board_button->turn == PLAYER_NULL;
        bool not_from_future = board->present <= game_state->present;
        bool owned_by_player = game_state->turn == board->owner;
        bool correct_time_flow = (time_traveled && board->has_next_board) ||
                                 (!time_traveled && !board->has_next_board);

        bool can_press = no_turn_taken && not_from_future && owned_by_player &&
                         correct_time_flow;

        if (!can_press)
            continue;

        struct transform *board_transform = NULL;
        error = ecs_world_query_get_single(
            (void **)&board_transform, query->world, board->background_entity,
            ECS_COMP_TRANSFORM);
        if (error)
            continue;

        float x = board_transform->pos[0];
        float y = board_transform->pos[1];

        u32 image_entity = UINT32_MAX;
        error = array_elem_get_cpy(&image_entity, node_data->children, 1);
        if (error)
            continue;

        struct renderable *ren = NULL;
        error = ecs_world_query_get_single((void **)&ren, query->world,
                                           image_entity, ECS_COMP_RENDERABLE);
        if (error || !ren)
            continue;

        enum player new_board_state[9] = {PLAYER_NULL};
        memcpy(new_board_state, board->state, sizeof(new_board_state));

        if (time_traveled) {
            game_state->present = board->present + 1;
            game_state->timelines += 1;
            y = (float)game_state->timelines * (624 + (624 * 0.5f));
        } else {
            board->has_next_board = true;
            game_state->present += 1;
            board->state[board_button->id] = board->owner;
            board_button->turn = board->owner;

            switch (board->owner) {
            case PLAYER_X:
                glm_ivec4_copy((ivec4){255, 100, 100, 255}, ren->tint);
                glm_vec2_copy((vec2){0.0f, 0.0f}, ren->uv_offset);
                break;
            case PLAYER_O:
                glm_ivec4_copy((ivec4){100, 100, 255, 255}, ren->tint);
                glm_vec2_copy((vec2){0.5f, 0.0f}, ren->uv_offset);
                break;
            case PLAYER_NONE:
            default:
                break;
            }
        }

        new_board_state[board_button->id] = board->owner;

        game_state->turn = game_state->turn == PLAYER_X ? PLAYER_O : PLAYER_X;
        enum player new_board_owner = game_state->turn;

        game_board_entity_new(query->world, new_board_owner,
                              (vec2){(float)x + 624 + (624 * 0.5f), y},
                              game_state->present, game_state->timelines,
                              new_board_state);

        struct renderable *present_bar_ren = NULL;
        error = ecs_world_query_get_single(
            (void **)&present_bar_ren, query->world,
            game_state->present_bar_entity, ECS_COMP_RENDERABLE);
        if (error || !present_bar_ren)
            continue;

        struct transform *present_bar_tf = NULL;
        error = ecs_world_query_get_single(
            (void **)&present_bar_tf, query->world,
            game_state->present_bar_entity, ECS_COMP_TRANSFORM);
        if (error || !present_bar_tf)
            continue;

        if (game_state->turn == PLAYER_X) {
            glm_ivec4_copy((ivec4){255, 100, 100, 255}, present_bar_ren->tint);
        } else if (game_state->turn == PLAYER_O) {
            glm_ivec4_copy((ivec4){100, 100, 255, 255}, present_bar_ren->tint);
        }

        present_bar_tf->pos[0] = x + 624 + (624 * 0.5f);
    }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_MAIN_BOARD_BUTTON_H
