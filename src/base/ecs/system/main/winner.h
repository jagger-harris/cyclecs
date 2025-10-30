#ifndef ECS_SYSTEM_MAIN_WINNER_H
#define ECS_SYSTEM_MAIN_WINNER_H

#include "base/game/game.h"
#include "core/ecs/system/ui_button.h"

static enum player check_winner(enum player state[9]) {
    static const int wins[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Cols
        {0, 4, 8}, {2, 4, 6} // Diagonals
    };

    for (int i = 0; i < 8; ++i) {
        int a = wins[i][0], b = wins[i][1], c = wins[i][2];
        if (state[a] != PLAYER_NULL && state[a] == state[b] &&
            state[a] == state[c]) {
            return state[a];
        }
    }

    bool all_filled = true;
    for (int i = 0; i < 9; ++i) {
        if (state[i] == PLAYER_NULL) {
            all_filled = false;
            break;
        }
    }

    if (all_filled)
        return PLAYER_NONE;

    return PLAYER_NULL;
}

int main_winner_system(struct ecs_world_query *query, struct app *app) {
    if (!query || !query->world || !app)
        return CORE_NULLPTR;

    struct game_state *game_state = app->game_state;
    if (!game_state)
        return CORE_NULLPTR;

    u32 entity = U32_MAX;
    while (ecs_world_query_next(&entity, query) == CORE_SUCCESS &&
           entity != U32_MAX) {
        struct board *board = NULL;
        int error = ecs_world_query_get((void **)&board, query, GAME_COMP_BOARD,
                                        entity);
        if (error || !board)
            continue;

        enum player winner = check_winner(board->state);
        if (winner != PLAYER_NULL) {
            game_state->winner = winner;
            break;
        }
    }

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_MAIN_WINNER_H
