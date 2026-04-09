#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/ecs/system/button.h>
#include <cls/gfx/gfx_api.h>
#include <cls/gfx/gl/renderer.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/gl/texture2d.h>
#include <cls/gfx/texture2d.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/xxhash32.h>
#include <string.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define SCREEN_TITLE "Tic-Tac-Toe"
#define SCREEN_COLOR (ivec4){5, 10, 20, 255}

#define COMP_GAME_STATE "game_state"
#define COMP_CELL_STATE "cell_state"
#define COMP_BOARD_GROUP "board_group"

enum player { PLAYER_NONE = 0, PLAYER_X, PLAYER_O };
enum winner { WINNER_NONE = 0, WINNER_X, WINNER_O, WINNER_DRAW };

struct board_group {
    u8 _;
};

struct cell_state {
    int index;
    enum player owner;
};

struct game_state {
    enum player turn;
    enum winner winner;
};

struct winner_label_data {
    struct cls_ecs_world *main_world;
    cls_entity state_e;
};

static int preset_game_state_spawn(cls_entity *state,
                                   struct cls_ecs_world *world,
                                   enum player turn) {
    if (!world)
        return CLS_NULLPTR;

    cls_entity root = CLS_ENTITY_MAX;
    int error = cls_ecs_world_entity_add(&root, world);
    if (error)
        return error;

    error = cls_ecs_world_component_add(
        world, root, COMP_GAME_STATE,
        &(struct game_state){.turn = turn, .winner = WINNER_NONE});
    if (error)
        goto cleanup;

    if (state)
        *state = root;

    return CLS_SUCCESS;

cleanup:
    if (root != CLS_ENTITY_MAX)
        cls_ecs_world_entity_remove(world, root);

    return error;
}

static int preset_board_spawn(cls_entity *board, struct cls_ecs_world *world,
                              const char *id, vec2 pos, enum player turn) {
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

    cls_entity root = CLS_ENTITY_MAX;
    int error = cls_preset_rect_spawn(
        &root, world, "", pos, (vec2){board_border_size, board_border_size},
        0.0f, 0.0f, bg_tint, (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);
    if (error)
        goto cleanup;

    u32 id_hash = 0;
    error = cls_xxhash32(&id_hash, id, strlen(id), 0);
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

    error = cls_ecs_world_component_add(world, root, COMP_BOARD_GROUP,
                                        &(struct board_group){0});
    if (error)
        goto cleanup;

    float board_bg_size =
        gap * ((float)rows - 1.0f) + (button_size * (float)rows);

    cls_entity bg = CLS_ENTITY_MAX;
    error = cls_preset_rect_spawn(&bg, world, "", pos,
                                  (vec2){board_bg_size, board_bg_size}, 0.0f,
                                  0.25f, (ivec4){255, 255, 255, 255},
                                  (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);
    if (error)
        goto cleanup;

    const char *bg_id = "background";
    u32 bg_hash = 0;
    error = cls_xxhash32(&bg_hash, bg_id, strlen(bg_id), 0);
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(
        world, bg, CLS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = bg_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, bg, COMP_BOARD_GROUP,
                                        &(struct board_group){0});
    if (error)
        goto cleanup;

    for (int i = 0; i < rows * rows; ++i) {
        int col = i % rows;
        int row = i / rows;

        float x = pos[0] + start_x + (float)col * spacing;
        float y = pos[1] + start_y - (float)row * spacing;

        vec2 uv_offset = {0};
        ivec4 image_tint = {0};

        char buffer[8] = {0};
        int len = snprintf(buffer, sizeof(buffer), "%i", i);
        const char *button_id = buffer;

        cls_entity button = CLS_ENTITY_MAX;
        error = cls_preset_image_button_spawn(
            &button, world, button_id, "xo.png", (vec2){x, y}, 0.5f,
            (vec2){button_size, button_size}, uv_offset, (vec2){0.5f, 0.5f},
            image_tint, true);
        if (error)
            goto cleanup;

        error = cls_ecs_world_component_add(world, button, COMP_BOARD_GROUP,
                                            &(struct board_group){0});
        if (error)
            goto cleanup;

        error = cls_ecs_world_component_add(
            world, button, COMP_CELL_STATE,
            &(struct cell_state){.index = i, .owner = 0});
        if (error)
            goto cleanup;
    }

    if (board)
        *board = root;

    return CLS_SUCCESS;

cleanup:
    // TODO: Cleanup
    return error;
}

static int move_system(struct cls_ecs_world_query *query, struct cls_app *app,
                       void *user_data) {
    if (!query || !app || !user_data)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    int error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    cls_entity state_e = *(cls_entity *)user_data;
    struct game_state *state = NULL;

    error = cls_ecs_world_component_get((void **)&state, world, state_e,
                                        COMP_GAME_STATE);
    if (error)
        return error;

    if (!state)
        return CLS_NULLPTR;

    u32 root_hash = 0;
    const char *root_id = "root";
    error = cls_xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        return error;

    u32 sprite_hash = 0;
    const char *sprite_id = "sprite";
    error = cls_xxhash32(&sprite_hash, sprite_id, strlen(sprite_id), 0);
    if (error)
        return error;

    struct cell_ctx {
        u32 grp_id;
        struct button *button;
        struct cell_state *cell;
        struct renderable *ren;
    };

    struct cell_ctx roots[9] = {0};
    int root_count = 0;

    cls_entity sprite_entities[9];
    int sprite_count = 0;

    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS && e != U32_MAX) {
        void *grp_ptr = NULL;
        cls_ecs_world_query_component_get(&grp_ptr, query, e, CLS_COMP_GROUP);
        struct group *grp = grp_ptr;
        if (!grp)
            continue;

        if (grp->user_id == root_hash) {
            struct cell_ctx ctx = {0};
            ctx.grp_id = grp->grp_id;
            error = cls_ecs_world_component_get((void **)&ctx.button, world, e,
                                                CLS_COMP_BUTTON);
            if (error)
                continue;

            error = cls_ecs_world_component_get((void **)&ctx.cell, world, e,
                                                COMP_CELL_STATE);
            if (error)
                continue;

            error = cls_ecs_world_component_get((void **)&ctx.ren, world, e,
                                                CLS_COMP_RENDERABLE);
            if (error)
                continue;

            if (!ctx.button || !ctx.cell || !ctx.ren)
                continue;

            roots[root_count++] = ctx;

        } else if (grp->user_id == sprite_hash) {
            sprite_entities[sprite_count++] = e;
        }
    }

    for (int i = 0; i < sprite_count; ++i) {
        e = sprite_entities[i];

        void *grp_ptr = NULL;
        cls_ecs_world_query_component_get(&grp_ptr, query, e, CLS_COMP_GROUP);
        struct group *grp = grp_ptr;
        if (!grp)
            continue;

        struct cell_ctx *ctx = NULL;
        for (int j = 0; j < root_count; j++) {
            if (roots[j].grp_id == grp->grp_id) {
                ctx = &roots[j];
                break;
            }
        }

        if (!ctx)
            continue;

        // Hover tint
        if (ctx->button->hovering) {
            glm_ivec4_copy((ivec4){100, 100, 100, 255}, ctx->ren->tint);
        } else {
            glm_ivec4_copy((ivec4){0, 0, 0, 255}, ctx->ren->tint);
        }

        struct renderable *sprite_ren = NULL;
        cls_ecs_world_component_get((void **)&sprite_ren, world, e,
                                    CLS_COMP_RENDERABLE);
        if (!sprite_ren)
            continue;

        bool allow_click = !state->winner && ctx->button->released &&
                           ctx->cell->owner == PLAYER_NONE;

        // Clicking
        if (!allow_click)
            continue;

        switch (state->turn) {
        case PLAYER_X:
            ctx->cell->owner = PLAYER_X;
            glm_ivec4_copy((ivec4){255, 100, 100, 255}, sprite_ren->tint);
            glm_vec2_copy((vec2){0.0f, 0.0f}, sprite_ren->uv_offset);
            state->turn = PLAYER_O;
            break;
        case PLAYER_O:
            ctx->cell->owner = PLAYER_O;
            glm_ivec4_copy((ivec4){100, 100, 255, 255}, sprite_ren->tint);
            glm_vec2_copy((vec2){0.5f, 0.0f}, sprite_ren->uv_offset);
            state->turn = PLAYER_X;
            break;
        default:
            break;
        }
    }

    return CLS_SUCCESS;
}

static enum winner check_winner(enum player state[9]) {
    static const int wins[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Cols
        {0, 4, 8}, {2, 4, 6} // Diagonals
    };

    for (int i = 0; i < 8; ++i) {
        int a = wins[i][0], b = wins[i][1], c = wins[i][2];
        if (state[a] != PLAYER_NONE && state[a] == state[b] &&
            state[a] == state[c])
            return state[a] == PLAYER_X ? WINNER_X : WINNER_O;
    }

    for (int i = 0; i < 9; ++i)
        if (state[i] == PLAYER_NONE)
            return WINNER_NONE;

    return WINNER_DRAW;
}

static int winner_system(struct cls_ecs_world_query *query, struct cls_app *app,
                         void *user_data) {
    if (!query || !app || !user_data)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    int error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    cls_entity state_e = *(cls_entity *)user_data;
    struct game_state *state = NULL;
    error = cls_ecs_world_component_get((void **)&state, world, state_e,
                                        COMP_GAME_STATE);
    if (error)
        return error;

    if (!state)
        return CLS_NULLPTR;

    if (state->winner)
        return CLS_SUCCESS;

    enum player board[9] = {0};
    int cell_count = 0;

    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        void *cell_ptr = NULL;
        error =
            cls_ecs_world_component_get(&cell_ptr, world, e, COMP_CELL_STATE);
        if (error || !cell_ptr)
            continue;

        struct cell_state *cell = cell_ptr;
        if (!cell)
            continue;

        board[cell->index] = cell->owner;
    }

    enum winner winner = check_winner(board);
    if (winner != WINNER_NONE)
        state->winner = winner;

    return CLS_SUCCESS;
}

static int winner_label_system(struct cls_ecs_world_query *query,
                               struct cls_app *app, void *user_data) {
    if (!query || !app || !user_data)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    int error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    struct winner_label_data *winner_label_data = user_data;
    struct game_state *state = NULL;
    error = cls_ecs_world_component_get(
        (void **)&state, winner_label_data->main_world,
        winner_label_data->state_e, COMP_GAME_STATE);
    if (error)
        return error;

    if (!state)
        return CLS_NULLPTR;

    const char *winner_id = "winner";
    u32 winner_hash = 0;
    error = cls_xxhash32(&winner_hash, winner_id, strlen(winner_id), 0);
    if (error)
        return error;

    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS && e != U32_MAX) {
        struct group *grp = NULL;
        error = cls_ecs_world_query_component_get((void **)&grp, query, e,
                                                  CLS_COMP_GROUP);
        if (error)
            continue;

        if (!grp)
            continue;

        u32 id = grp->grp_id;
        if (grp->grp_id == winner_hash)
            break;
    }

    if (state->winner != WINNER_NONE) {
        cls_preset_group_despawn(e, world);

        error = cls_preset_label_spawn(
            NULL, world, app->assets, "winner", (vec2){100.0f, 500.0f}, 1.0f,
            "Someone won!", 150, "human_sans-regular.otf", true,
            (ivec4){255, 255, 255, 255});
        if (error)
            return error;
    }

    return CLS_SUCCESS;
}

static int debug_labels_system(struct cls_ecs_world_query *query,
                               struct cls_app *app, void *user_data) {
    if (!query || !app || !user_data)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    int error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    struct winner_label_data *winner_label_data = user_data;
    struct game_state *state = NULL;
    error = cls_ecs_world_component_get(
        (void **)&state, winner_label_data->main_world,
        winner_label_data->state_e, COMP_GAME_STATE);
    if (error)
        return error;

    if (!state)
        return CLS_NULLPTR;

    const char *fps_id = "fps";
    u32 fps_hash = 0;
    error = cls_xxhash32(&fps_hash, fps_id, strlen(fps_id), 0);
    if (error)
        return error;

    const char *entities_id = "entities";
    u32 entities_hash = 0;
    error = cls_xxhash32(&entities_hash, entities_id, strlen(entities_id), 0);
    if (error)
        return error;

    cls_entity fps_e = CLS_ENTITY_MAX;
    cls_entity entities_e = CLS_ENTITY_MAX;
    cls_entity e = CLS_ENTITY_MAX;
    while (cls_ecs_world_query_next(&e, query) == CLS_SUCCESS && e != U32_MAX) {
        struct group *grp = NULL;
        error = cls_ecs_world_query_component_get((void **)&grp, query, e,
                                                  CLS_COMP_GROUP);
        if (error)
            continue;

        if (!grp)
            continue;

        if (fps_e != CLS_ENTITY_MAX && entities_e != CLS_ENTITY_MAX)
            break;

        u32 id = grp->grp_id;
        if (grp->grp_id == fps_hash)
            fps_e = e;

        if (grp->grp_id == entities_hash)
            entities_e = e;
    }

    if (fps_e != CLS_ENTITY_MAX) {
        cls_preset_group_despawn(fps_e, world);

        float fps = 0.0f;
        error = cls_window_timing_fps_avg_get(&fps, app->window);
        if (error)
            return error;

        char fps_text[32];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", fps);

        error = cls_preset_label_spawn(NULL, world, app->assets, "fps",
                                       (vec2){10.0f, 30.0f}, 1.0f, fps_text, 20,
                                       "human_sans-regular.otf", true,
                                       (ivec4){255, 255, 255, 255});
        if (error)
            return error;
    }

    if (entities_e != CLS_ENTITY_MAX) {
        cls_preset_group_despawn(entities_e, world);

        error = cls_preset_label_spawn(
            NULL, world, app->assets, "entities", (vec2){10.0f, 50.0f}, 1.0f,
            "Entities: 0", 20, "human_sans-regular.otf", true,
            (ivec4){255, 255, 255, 255});
        if (error)
            return error;
    }

    return CLS_SUCCESS;
}

static int game_init(struct cls_app *app) {
    struct cls_assets *assets = app->assets;
    struct cls_ecs *ecs = app->ecs;

    // Assets
    cls_assets_texture2d_add(assets, "xo.png", CLS_TEXTURE_FILTER_LINEAR,
                             CLS_TEXTURE_WRAP_CLAMP);

    // Worlds
    int error = cls_ecs_world_add(ecs, "main", 0.0f, 0, true);
    if (error)
        return error;

    error = cls_ecs_world_add(ecs, "ui", 20.0f, 0, true);
    if (error)
        return error;

    struct cls_ecs_world *main_world = NULL;
    error = cls_ecs_world_get(&main_world, ecs, "main");
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_GAME_STATE,
                                             sizeof(struct game_state));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_BOARD_GROUP,
                                             sizeof(struct board_group));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_CELL_STATE,
                                             sizeof(struct cell_state));
    if (error)
        return error;

    // Singletons
    cls_entity state = CLS_ENTITY_MAX;
    error = preset_game_state_spawn(&state, main_world, PLAYER_X);
    if (error)
        return error;

    // Systems
    error = cls_ecs_world_system_add(main_world, "button", button_system, NULL,
                                     0, 3, CLS_COMP_UI, CLS_COMP_BUTTON,
                                     CLS_COMP_TRANSFORM);
    if (error)
        return error;

    error = cls_ecs_world_system_add(main_world, "move", move_system, &state,
                                     sizeof(state), 2, CLS_COMP_GROUP,
                                     CLS_COMP_BUTTON_GROUP);
    if (error)
        return error;

    error = cls_ecs_world_system_add(main_world, "winner", winner_system,
                                     &state, sizeof(state), 1, COMP_CELL_STATE);
    if (error)
        return error;

    struct cls_ecs_world *ui_world = NULL;
    error = cls_ecs_world_get(&ui_world, ecs, "ui");
    if (error)
        return error;

    struct winner_label_data winner_label_data = {.main_world = main_world,
                                                  .state_e = state};

    error = cls_ecs_world_system_add(
        ui_world, "winner_label", winner_label_system, &winner_label_data,
        sizeof(winner_label_data), 1, CLS_COMP_GROUP, CLS_COMP_LABEL_GROUP);
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, "debug_labels", debug_labels_system, &winner_label_data,
        sizeof(winner_label_data), 1, CLS_COMP_GROUP, CLS_COMP_LABEL_GROUP);
    if (error)
        return error;

    // Game
    ivec2 fb_size = {0};
    error = cls_window_fb_size_get(fb_size, app->window);
    if (error)
        return error;

    vec3 camera_start_pos = {-(float)fb_size[0] * 0.5f,
                             -(float)fb_size[1] * 0.5f, 0.0f};
    error = cls_preset_camera_ortho_spawn(
        NULL, main_world, camera_start_pos, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, 0, true);
    if (error)
        return error;

    for (int i = 0; i < 1; ++i) {
        error = preset_board_spawn(NULL, main_world, "board",
                                   (vec2){0.0f, 0.0f}, PLAYER_X);
        if (error)
            return error;
    }

    error = cls_preset_camera_ortho_spawn(
        NULL, ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, 1, true);
    if (error)
        return error;

    error = cls_preset_label_spawn(NULL, ui_world, app->assets, "fps",
                                   (vec2){10.0f, 30.0f}, 1.0f, "FPS: 100", 20,
                                   "human_sans-regular.otf", true,
                                   (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    error = cls_preset_label_spawn(NULL, ui_world, app->assets, "entities",
                                   (vec2){10.0f, 50.0f}, 1.0f, "Entities: 100",
                                   20, "human_sans-regular.otf", true,
                                   (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    return CLS_SUCCESS;
}

int main(void) {
    struct cls_gfx_api api =
        (struct cls_gfx_api){.init = cls_gl_renderer_init,
                             .swap_buffers = cls_gl_renderer_swap_buffers,
                             .on_resize = cls_gl_renderer_on_resize,
                             .begin_frame = cls_gl_renderer_begin_frame,
                             .draw_frame = cls_gl_renderer_draw_frame,
                             .shader_init = cls_gl_shader_init,
                             .shader_destroy = cls_gl_shader_destroy,
                             .shader_use = cls_gl_shader_use,
                             .texture2d_init = cls_gl_texture2d_init,
                             .texture2d_destroy = cls_gl_texture2d_destroy,
                             .texture2d_use = cls_gl_texture2d_use};
    struct cls_app app = {0};
    int error = cls_app_init(&app, &api, (ivec2){SCREEN_WIDTH, SCREEN_HEIGHT},
                             SCREEN_TITLE, SCREEN_COLOR);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s", "Init app failed");
        goto cleanup;
    }

    error = game_init(&app);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s", "Init game failed");
        goto cleanup;
    }

    error = cls_app_run(&app);
    if (error) {
        CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                             "Running app failed");
        goto cleanup;
    }

    cls_app_destroy(&app);
    return CLS_SUCCESS;

cleanup:
    CLS_LOGGER_LOG_ERROR(CLS_LOGGER_FATAL, error, "%s", "Fatal error");
    cls_app_destroy(&app);
    return error;
}
