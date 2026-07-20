#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/ecs/system/systems.h>
#include <cls/gfx/gl/renderer.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/gl/texture2d.h>
#include <cls/gfx/renderer.h>
#include <cls/gfx/texture2d.h>
#include <cls/util/logger.h>
#include <cls/util/profiler.h>
#include <cls/util/xxhash32.h>
#include <string.h>

static const int WIN_WIDTH = 500;
static const int WIN_HEIGHT = 500;
static const char *WIN_TITLE = "Tic-Tac-Toe";
static const bool WIN_VSYNC = false;
static const ivec4 WIN_COLOR = {5, 10, 20, 255};

enum world_ids { WORLD_MAIN = 0, WORLD_UI };
enum component_ids { COMP_BOARD_BUTTON, COMP_BOARD_TAG, COMP_CELL_STATE };
enum singleton_ids { SING_GAME_STATE, SING_MAIN_WORLD, SING_UI_WORLD };
enum system_ids {
    SYS_WINNER,
    SYS_CAMERA,
    SYS_RENDER,
    SYS_DEBUG_LABELS,
    SYS_LABEL_RENDER
};

enum textures { TEXTURE_XO = 1 };

enum player { PLAYER_NONE = 0, PLAYER_X, PLAYER_O };
enum winner { WINNER_NONE = 0, WINNER_X, WINNER_O, WINNER_DRAW };

struct board_tag {
    u8 _;
};

struct cell_state {
    int idx;
    enum player owner;
};

struct game_state {
    enum player turn;
    enum winner winner;
};

static cls_error preset_board_spawn(cls_entity *board,
                                    struct cls_ecs_world *world, const char *id,
                                    vec2 pos, enum player turn) {
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
    cls_error error = cls_preset_rect_spawn(
        &root, world, CLS_TEXTURE2D_BLANK, pos,
        (vec2){board_border_size, board_border_size}, 0.0f, 0.0f, bg_tint,
        (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);
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
        &(struct cls_group){.grp_id = id_hash, .user_id = root_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, root, COMP_BOARD_TAG,
                                        &(struct board_tag){0});
    if (error)
        goto cleanup;

    float board_bg_size =
        gap * ((float)rows - 1.0f) + (button_size * (float)rows);

    cls_entity bg = CLS_ENTITY_MAX;
    error = cls_preset_rect_spawn(&bg, world, CLS_TEXTURE2D_BLANK, pos,
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
        &(struct cls_group){.grp_id = id_hash, .user_id = bg_hash});
    if (error)
        goto cleanup;

    error = cls_ecs_world_component_add(world, bg, COMP_BOARD_TAG,
                                        &(struct board_tag){0});
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
            &button, world, button_id, TEXTURE_XO, (vec2){x, y}, 0.5f,
            (vec2){button_size, button_size}, uv_offset, (vec2){0.5f, 0.5f},
            image_tint, true);
        if (error)
            goto cleanup;

        error = cls_ecs_world_component_add(world, button, COMP_BOARD_TAG,
                                            &(struct board_tag){0});
        if (error)
            goto cleanup;

        error = cls_ecs_world_component_add(
            world, button, COMP_CELL_STATE,
            &(struct cell_state){.idx = i, .owner = 0});
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

static cls_error board_button_system(struct cls_ecs_world_query *query,
                                     struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    cls_error error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    void *state_ptr = NULL;
    error = cls_ecs_world_singleton_get(&state_ptr, world, SING_GAME_STATE);
    if (error)
        return error;

    struct game_state *state = state_ptr;

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
        struct cell_state *cell;
        struct cls_button *button;
        struct cls_renderable *ren;
    };

    struct cell_ctx roots[9] = {0};
    int root_count = 0;

    cls_entity sprite_entities[9];
    int sprite_count = 0;

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_group *grp = comps[0];

        if (grp->user_id == root_hash) {
            struct cell_ctx ctx = {.button = NULL, .cell = NULL, .ren = NULL};
            ctx.grp_id = grp->grp_id;

            void *button_ptr = NULL;
            error = cls_ecs_world_component_get(&button_ptr, world, e,
                                                CLS_COMP_BUTTON);
            if (error)
                continue;

            ctx.button = button_ptr;

            void *cell_ptr = NULL;
            error = cls_ecs_world_component_get(&cell_ptr, world, e,
                                                COMP_CELL_STATE);
            if (error)
                continue;

            ctx.cell = cell_ptr;

            void *ren_ptr = NULL;
            error = cls_ecs_world_component_get(&ren_ptr, world, e,
                                                CLS_COMP_RENDERABLE);
            if (error)
                continue;

            ctx.ren = ren_ptr;

            roots[root_count++] = ctx;
        } else if (grp->user_id == sprite_hash) {
            sprite_entities[sprite_count++] = e;
        }
    }

    for (int i = 0; i < sprite_count; ++i) {
        e = sprite_entities[i];

        void *grp_ptr = NULL;
        error = cls_ecs_world_component_get(&grp_ptr, world, e, CLS_COMP_GROUP);
        if (error)
            continue;

        struct cls_group *grp = grp_ptr;

        struct cell_ctx *ctx = NULL;
        for (int j = 0; j < root_count; ++j) {
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

        // Clicking
        bool allow_click = !state->winner && ctx->button->released &&
                           ctx->cell->owner == PLAYER_NONE;

        if (!allow_click)
            continue;

        void *sprite_ren_ptr = NULL;
        error = cls_ecs_world_component_get(&sprite_ren_ptr, world, e,
                                            CLS_COMP_RENDERABLE);
        if (error)
            continue;

        struct cls_renderable *sprite_ren = sprite_ren_ptr;

        switch (state->turn) {
        case PLAYER_X:
            ctx->cell->owner = PLAYER_X;
            state->turn = PLAYER_O;
            glm_ivec4_copy((ivec4){255, 100, 100, 255}, sprite_ren->tint);
            glm_vec2_copy((vec2){0.0f, 0.0f}, sprite_ren->uv_offset);
            break;
        case PLAYER_O:
            ctx->cell->owner = PLAYER_O;
            state->turn = PLAYER_X;
            glm_ivec4_copy((ivec4){100, 100, 255, 255}, sprite_ren->tint);
            glm_vec2_copy((vec2){0.5f, 0.0f}, sprite_ren->uv_offset);
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

static cls_error winner_system(struct cls_ecs_world_query *query,
                               struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    cls_error error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    void *state_ptr = NULL;
    error = cls_ecs_world_singleton_get(&state_ptr, world, SING_GAME_STATE);
    if (error)
        return error;

    struct game_state *state = state_ptr;

    if (state->winner)
        return CLS_SUCCESS;

    enum player board[9] = {0};
    int cell_count = 0;

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[1] = {NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        void *cell_ptr = NULL;
        error =
            cls_ecs_world_component_get(&cell_ptr, world, e, COMP_CELL_STATE);
        if (error)
            continue;

        struct cell_state *cell = cell_ptr;

        board[cell->idx] = cell->owner;
    }

    enum winner winner = check_winner(board);
    if (winner != WINNER_NONE) {
        state->winner = winner;

        struct cls_ecs_world *ui_world = NULL;
        error = cls_ecs_world_get(&ui_world, app->ecs, WORLD_UI);
        if (error)
            return error;

        error = cls_preset_label_spawn(
            NULL, ui_world, "winner", (vec2){100.0f, 100.0f}, 1.0f, "Winner!",
            100, CLS_FONT_HUMANSANS_REG, true, (ivec4){255, 255, 255, 255});
        if (error) {

            CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s", "Okay...");
            return error;
        }
    }

    return CLS_SUCCESS;
}

static cls_error debug_labels_system(struct cls_ecs_world_query *query,
                                     struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    cls_error error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    void *world_ptr = NULL;
    error = cls_ecs_world_singleton_get(&world_ptr, world, SING_MAIN_WORLD);
    if (error)
        return error;

    struct cls_ecs_world *main_world = *(struct cls_ecs_world **)world_ptr;

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

    const char *ram_id = "ram";
    u32 ram_hash = 0;
    error = cls_xxhash32(&ram_hash, ram_id, strlen(ram_id), 0);
    if (error)
        return error;

    struct cls_label *fps_l = NULL;
    struct cls_label *entities_l = NULL;
    struct cls_label *ram_l = NULL;
    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_group *grp = comps[0];
        struct cls_label *l = comps[1];

        if (fps_l != NULL && entities_l != NULL && ram_l != NULL)
            break;

        u32 id = grp->grp_id;
        if (grp->grp_id == fps_hash)
            fps_l = l;

        if (grp->grp_id == entities_hash)
            entities_l = l;

        if (grp->grp_id == ram_hash)
            ram_l = l;
    }

    if (!fps_l || !entities_l || !ram_l)
        return CLS_FAILURE;

    float fps = 0.0f;
    error = cls_window_timing_fps_avg_get(&fps, app->window);
    if (error)
        return error;

    snprintf(fps_l->text, sizeof(fps_l->text), "FPS: %.0f", fps);

    size_t main_world_entities_total = 0;
    error = cls_ecs_world_entities_length_get(&main_world_entities_total,
                                              main_world);
    if (error)
        return error;

    size_t main_world_entities_free = 0;
    error = cls_ecs_world_free_entities_length_get(&main_world_entities_free,
                                                   main_world);
    if (error)
        return error;

    size_t main_world_entities_active =
        main_world_entities_total - main_world_entities_free;

    size_t ui_world_entities_total = 0;
    error = cls_ecs_world_entities_length_get(&ui_world_entities_total, world);
    if (error)
        return error;

    size_t ui_world_entities_free = 0;
    error =
        cls_ecs_world_free_entities_length_get(&ui_world_entities_free, world);
    if (error)
        return error;

    size_t ui_world_entities_active =
        ui_world_entities_total - ui_world_entities_free;

    snprintf(entities_l->text, sizeof(entities_l->text), "ENTITIES: %.0zu",
             main_world_entities_active + ui_world_entities_active);

    size_t ram = 0;
    error = cls_profiler_mem_usage_get(&ram);
    if (error)
        return error;

    snprintf(ram_l->text, sizeof(ram_l->text), "RAM: %.0zuMB", ram);
    return CLS_SUCCESS;
}

static cls_error game_init(struct cls_app *app) {
    struct cls_assets *assets = app->assets;
    struct cls_ecs *ecs = app->ecs;

    // Assets
    cls_assets_texture2d_add(assets, TEXTURE_XO, "xo.png",
                             CLS_TEXTURE_FILTER_LINEAR, CLS_TEXTURE_WRAP_CLAMP);

    // Worlds
    struct cls_ecs_world *main_world = NULL;
    struct cls_ecs_world *ui_world = NULL;

    cls_error error = cls_ecs_world_add(&main_world, ecs, WORLD_MAIN, true);
    if (error)
        return error;

    error = cls_ecs_world_add(&ui_world, ecs, WORLD_UI, true);
    if (error)
        return error;

    error = cls_ecs_world_get(&main_world, ecs, WORLD_MAIN);
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_BOARD_TAG,
                                             sizeof(struct board_tag));
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_CELL_STATE,
                                             sizeof(struct cell_state));
    if (error)
        return error;

    // Singletons
    struct game_state state = {.turn = PLAYER_X, .winner = WINNER_NONE};
    error = cls_ecs_world_singleton_add(main_world, SING_GAME_STATE, &state,
                                        sizeof(struct game_state));
    if (error)
        return error;

    error = cls_ecs_world_singleton_add(main_world, SING_UI_WORLD, &ui_world,
                                        sizeof(ui_world));
    if (error)
        return error;

    error = cls_ecs_world_singleton_add(ui_world, SING_MAIN_WORLD, &main_world,
                                        sizeof(main_world));
    if (error)
        return error;

    // Systems
    error = cls_ecs_world_system_add(
        main_world, CLS_COMP_BUTTON, cls_button_system, 0.0f, 3,
        (const cls_component[]){CLS_COMP_UI, CLS_COMP_BUTTON,
                                CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        main_world, COMP_BOARD_BUTTON, board_button_system, 0.0f, 2,
        (const cls_component[]){CLS_COMP_GROUP, CLS_COMP_BUTTON_GROUP});
    if (error)
        return error;

    error =
        cls_ecs_world_system_add(main_world, SYS_WINNER, winner_system, 5.0f, 1,
                                 (const cls_component[]){COMP_CELL_STATE});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        main_world, SYS_CAMERA, cls_camera_system, 0.0f, 3,
        (const cls_component[]){CLS_COMP_CAMERA, CLS_COMP_CAMERA_ACTIVE,
                                CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        main_world, SYS_RENDER, cls_render_system, 0.0f, 2,
        (const cls_component[]){CLS_COMP_RENDERABLE, CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, SYS_DEBUG_LABELS, debug_labels_system, 5.0f, 2,
        (const cls_component[]){CLS_COMP_GROUP, CLS_COMP_LABEL});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, SYS_CAMERA, cls_camera_system, 0.0f, 3,
        (const cls_component[]){CLS_COMP_CAMERA, CLS_COMP_CAMERA_ACTIVE,
                                CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, SYS_LABEL_RENDER, cls_label_render_system, 0.0f, 2,
        (const cls_component[]){CLS_COMP_LABEL, CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, SYS_RENDER, cls_render_system, 0.0f, 2,
        (const cls_component[]){CLS_COMP_RENDERABLE, CLS_COMP_TRANSFORM});
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

    error = preset_board_spawn(NULL, main_world, "board", (vec2){0.0f, 0.0f},
                               PLAYER_X);
    if (error)
        return error;

    error = cls_preset_camera_ortho_spawn(
        NULL, ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, 1, true);
    if (error)
        return error;

    error = cls_preset_label_spawn(NULL, ui_world, "fps", (vec2){10.0f, 30.0f},
                                   1.0f, "FPS: 0", 20, CLS_FONT_HUMANSANS_REG,
                                   true, (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    error = cls_preset_label_spawn(
        NULL, ui_world, "entities", (vec2){10.0f, 50.0f}, 1.0f, "ENTITIES: 0",
        20, CLS_FONT_HUMANSANS_REG, true, (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    return cls_preset_label_spawn(NULL, ui_world, "ram", (vec2){10.0f, 70.0f},
                                  1.0f, "RAM: 0MB", 20, CLS_FONT_HUMANSANS_REG,
                                  true, (ivec4){255, 255, 255, 255});
}

int main(void) {
    struct cls_renderer_api api =
        (struct cls_renderer_api){.init = cls_gl_renderer_init,
                                  .swap_buffers = cls_gl_renderer_swap_buffers,
                                  .on_resize = cls_gl_renderer_on_resize,
                                  .begin_frame = cls_gl_renderer_begin_frame,
                                  .draw_batches = cls_gl_renderer_draw_batches,
                                  .shader_init = cls_gl_shader_init,
                                  .shader_destroy = cls_gl_shader_destroy,
                                  .shader_use = cls_gl_shader_use,
                                  .texture2d_init = cls_gl_texture2d_init,
                                  .texture2d_destroy = cls_gl_texture2d_destroy,
                                  .texture2d_use = cls_gl_texture2d_use};
    struct cls_app app = {0};
    cls_error error = cls_app_init(&app, &api, (ivec2){WIN_WIDTH, WIN_HEIGHT},
                                   WIN_TITLE, WIN_VSYNC, WIN_COLOR);
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
