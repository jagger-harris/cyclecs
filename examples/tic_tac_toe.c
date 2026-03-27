#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/ecs/system/button.h>
#include <cls/gfx/api.h>
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

enum player { PLAYER_NONE = 1, PLAYER_X, PLAYER_O };

struct board {
    enum player state[9];
    enum player turn;
};

struct board_group {
    u8 _;
};

struct board_button {
    enum player turn;
};

struct state {
    enum player turn;
    enum player winner;
};

static int preset_board_spawn(entity *board, struct ecs_world *world,
                              const char *id, vec2 pos, enum player state[9],
                              enum player turn) {
    int rows = 1;
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

    entity root = ENTITY_MAX;
    int error = preset_rect_spawn(
        &root, world, "", pos, (vec2){board_border_size, board_border_size},
        0.0f, 0.0f, bg_tint, (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);
    if (error)
        goto cleanup;

    u32 id_hash = 0;
    error = xxhash32(&id_hash, id, strlen(id), 0);
    if (error)
        return error;

    const char *root_id = "root";
    u32 root_hash = 0;
    error = xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(
        world, root, CLS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, "board_group",
                                    &(struct board_group){0});
    if (error)
        goto cleanup;

    struct board board_comp = {.turn = turn};
    memcpy(board_comp.state, state, sizeof(board_comp.state));

    error = ecs_world_component_add(world, root, "board", &board_comp);
    if (error)
        goto cleanup;

    float board_bg_size =
        gap * ((float)rows - 1.0f) + (button_size * (float)rows);

    entity bg = ENTITY_MAX;
    error = preset_rect_spawn(&bg, world, "", pos,
                              (vec2){board_bg_size, board_bg_size}, 0.0f, 0.25f,
                              (ivec4){255, 255, 255, 255}, (vec2){1.0f, 1.0f},
                              (vec2){1.0f, 1.0f}, true);
    if (error)
        goto cleanup;

    const char *bg_id = "background";
    u32 bg_hash = 0;
    error = xxhash32(&bg_hash, bg_id, strlen(bg_id), 0);
    if (error)
        goto cleanup;

    error = ecs_world_component_add(
        world, bg, CLS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = bg_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, bg, "board_group",
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

        if (board_comp.state[i] == PLAYER_X) {
            glm_ivec4_copy((ivec4){255, 100, 100, 255}, image_tint);
        } else if (board_comp.state[i] == PLAYER_O) {
            glm_vec2_copy((vec2){0.5f, 0.0f}, uv_offset);
            glm_ivec4_copy((ivec4){100, 100, 255, 255}, image_tint);
        }

        entity button = ENTITY_MAX;
        error = preset_image_button_spawn(
            &button, world, "board_button", "xo.png", (vec2){x, y}, 0.5f,
            (vec2){button_size, button_size}, uv_offset, (vec2){0.5f, 0.5f},
            image_tint, true);
        if (error)
            goto cleanup;

        char buffer[8];
        int len = snprintf(buffer, sizeof(buffer), "%i", i);
        const char *button_id = buffer;

        error = ecs_world_component_add(world, button, "board_group",
                                        &(struct board_group){0});
        if (error)
            goto cleanup;

        error = ecs_world_component_add(
            world, button, "board_button",
            &(struct board_button){.turn = board_comp.state[i]});
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

static enum player check_winner(enum player state[9]) {
    static const int wins[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // Rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // Cols
        {0, 4, 8}, {2, 4, 6} // Diagonals
    };

    for (int i = 0; i < 8; ++i) {
        int a = wins[i][0];
        int b = wins[i][1];
        int c = wins[i][2];

        if (state[a] != PLAYER_NONE && state[a] == state[b] &&
            state[a] == state[c]) {
            return state[a];
        }
    }

    bool all_filled = true;
    for (int i = 0; i < 9; ++i) {
        if (state[i] == PLAYER_NONE) {
            all_filled = false;
            break;
        }
    }

    if (all_filled)
        return PLAYER_NONE;

    return 0;
}

static int board_button_system(struct ecs_world_query *query, struct app *app,
                               void *user_data) {
    (void)user_data;

    if (!query || !app)
        return CLS_NULLPTR;

    struct ecs_world *world = NULL;
    int error = ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    struct ecs_world_query *board_query = NULL;
    error = ecs_world_query_create(&board_query, world, 1, "board");
    if (error)
        return error;

    entity board_e = ENTITY_MAX;
    error = ecs_world_query_next(&board_e, board_query);
    if (error)
        return error;

    void *board_ptr = NULL;
    error = ecs_world_query_component_get(&board_ptr, board_query, "board",
                                          board_e);
    if (error)
        return error;

    struct board *board_comp = board_ptr;
    ecs_world_query_destroy(board_query);

    const char *root_id = "root";
    u32 root_hash = 0;
    error = xxhash32(&root_hash, root_id, strlen(root_id), 0);
    if (error)
        return error;

    const char *sprite_id = "sprite";
    u32 sprite_hash = 0;
    error = xxhash32(&sprite_hash, sprite_id, strlen(sprite_id), 0);
    if (error)
        return error;

    struct button *button_comp = NULL;

    entity e = ENTITY_MAX;
    while (ecs_world_query_next(&e, query) == CLS_SUCCESS && e != U32_MAX) {
        void *grp_ptr = NULL;
        ecs_world_query_component_get(&grp_ptr, query, CLS_COMP_GROUP, e);
        struct group *grp = grp_ptr;
        if (!grp)
            continue;

        if (grp->user_id == root_hash) {
            void *button_comp_ptr = NULL;
            ecs_world_component_get(&button_comp_ptr, world, e,
                                    CLS_COMP_BUTTON);

            button_comp = button_comp_ptr;
            if (!button_comp)
                continue;

            void *ren_comp_ptr = NULL;
            ecs_world_component_get(&ren_comp_ptr, world, e,
                                    CLS_COMP_RENDERABLE);

            struct renderable *ren_comp = ren_comp_ptr;
            if (!ren_comp)
                continue;

            if (button_comp->hovering) {
                glm_ivec4_copy((ivec4){100, 100, 100, 255}, ren_comp->tint);
            } else {
                glm_ivec4_copy((ivec4){0, 0, 0, 255}, ren_comp->tint);
            }

            continue;
        }

        if (grp->user_id == sprite_hash) {
            if (!button_comp)
                continue;

            void *ren_comp_ptr = NULL;
            ecs_world_component_get(&ren_comp_ptr, world, e,
                                    CLS_COMP_RENDERABLE);

            struct renderable *ren_comp = ren_comp_ptr;
            if (!ren_comp || !board_comp)
                continue;

            if (button_comp->released) {
                switch (board_comp->turn) {
                case PLAYER_X:
                    glm_ivec4_copy((ivec4){255, 100, 100, 255}, ren_comp->tint);
                    glm_vec2_copy((vec2){0.0f, 0.0f}, ren_comp->uv_offset);
                    board_comp->turn = PLAYER_O;
                    break;
                case PLAYER_O:
                    glm_ivec4_copy((ivec4){100, 100, 255, 255}, ren_comp->tint);
                    glm_vec2_copy((vec2){0.5f, 0.0f}, ren_comp->uv_offset);
                    board_comp->turn = PLAYER_X;
                    break;
                case PLAYER_NONE:
                default:
                    break;
                }
            }
        }
    }

    return CLS_SUCCESS;
}

static int winner_system(struct ecs_world_query *query, struct app *app,
                         void *user_data) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct state *state = user_data;
    entity board = ENTITY_MAX;
    while (ecs_world_query_next(&board, query) == CLS_SUCCESS &&
           board != U32_MAX) {
        void *board_comp_ptr = NULL;
        int error = ecs_world_query_component_get(&board_comp_ptr, query,
                                                  "board", board);
        if (error)
            continue;

        struct board *board_comp = board_comp_ptr;
        if (!board_comp_ptr)
            continue;

        enum player winner = check_winner(board_comp->state);
        if (winner != 0) {
            state->winner = winner;
            break;
        }
    }

    return CLS_SUCCESS;
}

static int game_init(struct state *state, struct app *app) {
    struct assets *assets = app->assets;
    struct ecs *ecs = app->ecs;
    state->turn = PLAYER_X;
    state->winner = PLAYER_NONE;

    // Assets
    assets_texture2d_add(assets, "xo.png", CLS_TEXTURE_FILTER_LINEAR,
                         CLS_TEXTURE_WRAP_CLAMP);

    // Worlds
    int error = ecs_world_add(ecs, "main", 0.0f, 0, true);
    if (error)
        return error;

    error = ecs_world_add(ecs, "ui", 20.0f, 0, true);
    if (error)
        return error;

    struct ecs_world *main_world = NULL;
    error = ecs_world_get(&main_world, ecs, "main");
    if (error)
        return error;

    error = ecs_world_component_type_add(main_world, "board_group",
                                         sizeof(struct board_group));
    if (error)
        return error;

    error =
        ecs_world_component_type_add(main_world, "board", sizeof(struct board));
    if (error)
        return error;

    error = ecs_world_component_type_add(main_world, "board_button",
                                         sizeof(struct board_button));
    if (error)
        return error;

    // Systems
    error =
        ecs_world_system_add(main_world, "button", button_system, NULL, 3,
                             CLS_COMP_UI, CLS_COMP_BUTTON, CLS_COMP_TRANSFORM);
    if (error)
        return error;

    error =
        ecs_world_system_add(main_world, "board_button", board_button_system,
                             NULL, 2, CLS_COMP_GROUP, CLS_COMP_BUTTON_GROUP);
    if (error)
        return error;

    error = ecs_world_system_add(main_world, "winner", winner_system, state, 1,
                                 "board");
    if (error)
        return error;

    struct ecs_world *ui_world = NULL;
    error = ecs_world_get(&ui_world, ecs, "ui");
    if (error)
        return error;

    // Game
    ivec2 fb_size = {0};
    error = window_fb_size_get(fb_size, app->window);
    if (error)
        return error;

    vec3 camera_start_pos = {-(float)fb_size[0] * 0.5f,
                             -(float)fb_size[1] * 0.5f, 0.0f};
    error = preset_camera_ortho_spawn(NULL, main_world, camera_start_pos, 0.0f,
                                      (float)fb_size[0], (float)fb_size[1],
                                      0.0f, 1.0f, -1.0f, 1.0f, true, true);
    if (error)
        return error;

    enum player board_state[9] = {PLAYER_NONE};
    for (int i = 0; i < 1; ++i) {
        error = preset_board_spawn(NULL, main_world, "board",
                                   (vec2){0.0f, 0.0f}, board_state, PLAYER_X);
        if (error)
            return error;
    }

    error = preset_camera_ortho_spawn(
        NULL, ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, true);
    if (error)
        return error;

    error = preset_label_spawn(
        NULL, ui_world, assets, "winner", (vec2){100.0f, 500.0f}, 1.0f,
        "this is some test text", 150, "human_sans-regular.otf", true,
        (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    return CLS_SUCCESS;
}

int main(void) {
    struct gfx_api api =
        (struct gfx_api){.init = gl_renderer_init,
                         .swap_buffers = gl_renderer_swap_buffers,
                         .on_resize = gl_renderer_on_resize,
                         .draw_frame = gl_renderer_draw_frame,
                         .shader_init = gl_shader_init,
                         .shader_destroy = gl_shader_destroy,
                         .shader_use = gl_shader_use,
                         .texture2d_init = gl_texture2d_init,
                         .texture2d_destroy = gl_texture2d_destroy,
                         .texture2d_use = gl_texture2d_use};
    struct state state = {0};
    struct app app = {0};
    int error = app_init(&app, &api, (ivec2){SCREEN_WIDTH, SCREEN_HEIGHT},
                         SCREEN_TITLE, SCREEN_COLOR);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init app failed");
        goto cleanup;
    }

    error = game_init(&state, &app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Init game failed");
        goto cleanup;
    }

    error = app_run(&app);
    if (error) {
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s", "Running app failed");
        goto cleanup;
    }

    app_destroy(&app);
    return CLS_SUCCESS;

cleanup:
    LOGGER_LOG_ERROR(LOGGER_FATAL, error, "%s", "Fatal error");
    app_destroy(&app);
    return error;
}
