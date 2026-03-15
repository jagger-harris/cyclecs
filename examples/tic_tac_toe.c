#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/component/camera.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/ecs/system/ui_button.h>
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
};

struct board_group {
    u8 _;
};

struct board_button {
    enum player turn;
};

struct game_state {
    enum player turn;
    enum player winner;
};

enum ecs_worlds { GAME_WORLD_MAIN = 0, GAME_WORLD_UI };
enum comp_types {
    COMP_BOARD = CLS_ECS_COMP_LENGTH,
    COMP_BOARD_GROUP,
    COMP_BOARD_BUTTON,
    COMP_ARROW
};
enum game_sys {
    SYS_CAMERA = CLS_ECS_SYS_LENGTH,
    SYS_UI_BUTTON,
    SYS_BOARD_BUTTON,
    SYS_WINNER,
    SYS_BENCHMARK,
    SYS_SAVE_SCENE,
};

static int preset_board_spawn(entity *board, struct ecs_world *world,
                              const char *id, vec2 pos, enum player state[9]) {
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
        world, root, CLS_ECS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, root, COMP_BOARD_GROUP,
                                    &(struct board_group){0});
    if (error)
        goto cleanup;

    struct board board_comp = {0};
    memcpy(board_comp.state, state, sizeof(board_comp.state));

    error = ecs_world_component_add(world, root, COMP_BOARD, &board);
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
        world, bg, CLS_ECS_COMP_GROUP,
        &(struct group){.grp_id = id_hash, .user_id = bg_hash});
    if (error)
        goto cleanup;

    error = ecs_world_component_add(world, bg, COMP_BOARD_GROUP,
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
        error = preset_ui_image_button_spawn(
            &button, world, "board_button", "xo.png", (vec2){x, y}, 0.5f,
            (vec2){button_size, button_size}, uv_offset, (vec2){0.5f, 0.5f},
            image_tint, true);
        if (error)
            goto cleanup;

        char buffer[8];
        int len = snprintf(buffer, sizeof(buffer), "%i", i);
        const char *button_id = buffer;

        u32 button_hash = 0;
        error = xxhash32(&button_hash, button_id, len, 0);
        if (error)
            goto cleanup;

        error = ecs_world_component_add(
            world, button, CLS_ECS_COMP_GROUP,
            &(struct group){.grp_id = id_hash, .user_id = button_hash});
        if (error)
            goto cleanup;

        error = ecs_world_component_add(world, button, COMP_BOARD_GROUP,
                                        &(struct board_group){0});
        if (error)
            goto cleanup;

        error = ecs_world_component_add(
            world, button, COMP_BOARD_BUTTON,
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

static int add_component_types(struct ecs_world *world, void *user_comp) {
    (void)user_comp;

    int error = ecs_world_component_type_add(world, CLS_ECS_COMP_GROUP,
                                             sizeof(struct group));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_TRANSFORM,
                                         sizeof(struct transform));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_RENDERABLE,
                                         sizeof(struct renderable));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_CAMERA,
                                         sizeof(struct camera));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_CAMERA_ACTIVE,
                                         sizeof(struct camera_active));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_UI_BASE,
                                         sizeof(struct ui_base));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_UI_BUTTON,
                                         sizeof(struct ui_button));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_UI_BUTTON_GROUP,
                                         sizeof(struct ui_button_group));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_UI_LABEL,
                                         sizeof(struct ui_label));
    if (error)
        return error;

    error = ecs_world_component_type_add(world, CLS_ECS_COMP_UI_LABEL_GROUP,
                                         sizeof(struct ui_label_group));
    if (error)
        return error;

    return CLS_SUCCESS;
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
                               void *user_comp) {
    (void)user_comp;

    if (!query || !app)
        return CLS_NULLPTR;

    struct ecs_world *world = NULL;
    int error = ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    entity button = ENTITY_MAX;
    while (ecs_world_query_next(&button, query) == CLS_SUCCESS &&
           button != U32_MAX) {
        void *button_comp_ptr = NULL;
        int error = ecs_world_query_component_get(
            &button_comp_ptr, query, CLS_ECS_COMP_UI_BUTTON, button);

        struct ui_button *button_comp = button_comp_ptr;
        if (!button)
            continue;

        void *bg_ren_comp_ptr = NULL;
        error = ecs_world_query_component_get(&bg_ren_comp_ptr, query,
                                              CLS_ECS_COMP_RENDERABLE, button);
        if (!bg_ren_comp_ptr)
            continue;

        struct renderable *bg_ren_comp = bg_ren_comp_ptr;
        if (!bg_ren_comp)
            continue;

        if (button_comp->hovering) {
            glm_ivec4_copy((ivec4){100, 100, 100, 255}, bg_ren_comp->tint);
        } else {
            glm_ivec4_copy((ivec4){0, 0, 0, 255}, bg_ren_comp->tint);
        }
    }

    return CLS_SUCCESS;
}

static int winner_system(struct ecs_world_query *query, struct app *app,
                         void *user_data) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct game_state *state = user_data;
    entity board = ENTITY_MAX;
    while (ecs_world_query_next(&board, query) == CLS_SUCCESS &&
           board != U32_MAX) {
        void *board_comp_ptr = NULL;
        int error = ecs_world_query_component_get(&board_comp_ptr, query,
                                                  COMP_BOARD, board);
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

static int game_init(struct game_state *state, struct app *app) {
    struct assets *assets = app->assets;
    struct ecs *ecs = app->ecs;
    state->turn = PLAYER_X;
    state->winner = PLAYER_NONE;

    // Assets
    assets_texture2d_add(assets, "xo.png", TEXTURE_FILTER_LINEAR,
                         TEXTURE_WRAP_CLAMP);

    // Components
    int error = ecs_world_add(ecs, GAME_WORLD_MAIN, 0.0f, 0, true);
    if (error)
        return error;

    error = ecs_world_add(ecs, GAME_WORLD_UI, 20.0f, 0, true);
    if (error)
        return error;

    error = ecs_world_iter_all(ecs, add_component_types, NULL);
    if (error)
        return error;

    struct ecs_world *main_world = NULL;
    error = ecs_world_get(&main_world, ecs, GAME_WORLD_MAIN);
    if (error)
        return error;

    error = ecs_world_component_type_add(main_world, COMP_BOARD_GROUP,
                                         sizeof(struct board_group));
    if (error)
        return error;

    error = ecs_world_component_type_add(main_world, COMP_BOARD,
                                         sizeof(struct board));
    if (error)
        return error;

    error = ecs_world_component_type_add(main_world, COMP_BOARD_BUTTON,
                                         sizeof(struct board_button));
    if (error)
        return error;

    // Systems
    error = ecs_world_system_add(
        main_world, CLS_ECS_SYS_UI_BUTTON, ui_button_system, NULL, 3,
        CLS_ECS_COMP_UI_BASE, CLS_ECS_COMP_UI_BUTTON, CLS_ECS_COMP_TRANSFORM);
    if (error)
        return error;

    error = ecs_world_system_add(
        main_world, SYS_BOARD_BUTTON, board_button_system, NULL, 2,
        CLS_ECS_COMP_RENDERABLE, CLS_ECS_COMP_UI_BUTTON);
    if (error)
        return error;

    error = ecs_world_system_add(main_world, SYS_WINNER, winner_system, state,
                                 1, COMP_BOARD);
    if (error)
        return error;

    struct ecs_world *ui_world = NULL;
    error = ecs_world_get(&ui_world, ecs, GAME_WORLD_UI);
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
                                   (vec2){0.0f, 0.0f}, board_state);
        if (error)
            return error;
    }

    error = preset_camera_ortho_spawn(
        NULL, ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, true);
    if (error)
        return error;

    error = preset_ui_label_spawn(
        NULL, ui_world, assets, "winner_text", (vec2){100.0f, 500.0f}, 1.0f, "",
        150, "human_sans-regular.otf", true, (ivec4){255, 255, 255, 255});
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
    struct game_state state = {0};
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
