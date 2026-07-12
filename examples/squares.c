#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/preset/presets.h>
#include <cls/ecs/system/systems.h>
#include <cls/gfx/gfx_api.h>
#include <cls/gfx/gl/renderer.h>
#include <cls/gfx/gl/shader.h>
#include <cls/gfx/gl/texture2d.h>
#include <cls/gfx/texture2d.h>
#include <cls/util/array.h>
#include <cls/util/error.h>
#include <cls/util/logger.h>
#include <cls/util/profiler.h>
#include <cls/util/xxhash32.h>

static const int WIN_WIDTH = 500;
static const int WIN_HEIGHT = 500;
static const char *WIN_TITLE = "Squares";
static const ivec4 WIN_COLOR = {5, 10, 20, 255};

static const char *COMP_SQUARE_TAG = "square_tag";

struct square_tag {
    u8 _;
};

struct worlds {
    struct cls_ecs_world *main;
};

static int debug_labels_system(struct cls_ecs_world_query *query,
                               struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    int error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    void *worlds_ptr = NULL;
    error = cls_ecs_world_singleton_get(&worlds_ptr, world, "worlds");
    if (error)
        return error;

    struct worlds *worlds = worlds_ptr;

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

    struct label *fps_l = NULL;
    struct label *entities_l = NULL;
    struct label *ram_l = NULL;
    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct group *grp = comps[0];
        struct label *l = comps[1];

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
                                              worlds->main);
    if (error)
        return error;

    size_t main_world_entities_free = 0;
    error = cls_ecs_world_free_entities_length_get(&main_world_entities_free,
                                                   worlds->main);
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

static int squares_system(struct cls_ecs_world_query *query,
                          struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    float dt = 0.0f;
    int error = cls_window_timing_dt_get(&dt, app->window);
    if (error)
        return error;

    float speed = 10.0f;
    float dtheta = speed * dt;

    float cos_t = cosf(dtheta);
    float sin_t = sinf(dtheta);

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct transform *tf = comps[1];
        float x = tf->pos[0];
        float y = tf->pos[1];
        float z = tf->pos[2];

        float x_new = x * cos_t - y * sin_t;
        float y_new = x * sin_t + y * cos_t;
        glm_vec3_copy((vec3){x_new, y_new, z}, tf->pos);
        tf->dirty = true;
    }

    return CLS_SUCCESS;
}

static int game_init(struct cls_app *app) {
    struct cls_ecs *ecs = app->ecs;

    // Worlds
    struct cls_ecs_world *main_world = NULL;
    struct cls_ecs_world *ui_world = NULL;

    int error = cls_ecs_world_add(&main_world, ecs, "main", true);
    if (error)
        return error;

    error = cls_ecs_world_add(&ui_world, ecs, "ui", true);
    if (error)
        return error;

    error = cls_ecs_world_component_type_add(main_world, COMP_SQUARE_TAG,
                                             sizeof(struct square_tag));
    if (error)
        return error;

    // Singletons
    struct worlds worlds = {.main = main_world};
    error = cls_ecs_world_singleton_add(ui_world, "worlds", &worlds,
                                        sizeof(struct worlds));
    if (error)
        return error;

    // Systems
    error = cls_ecs_world_system_add(
        main_world, "squares", squares_system, 0.0f, 2,
        (const char *[]){COMP_SQUARE_TAG, CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        main_world, "camera", cls_camera_system, 0.0f, 3,
        (const char *[]){CLS_COMP_CAMERA, CLS_COMP_CAMERA_ACTIVE,
                         CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        main_world, "render", cls_render_system, 0.0f, 2,
        (const char *[]){CLS_COMP_RENDERABLE, CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, "debug_labels", debug_labels_system, 5.0f, 2,
        (const char *[]){CLS_COMP_GROUP, CLS_COMP_LABEL});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, "camera", cls_camera_system, 0.0f, 3,
        (const char *[]){CLS_COMP_CAMERA, CLS_COMP_CAMERA_ACTIVE,
                         CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, "label_render", cls_label_render_system, 0.0f, 2,
        (const char *[]){CLS_COMP_LABEL, CLS_COMP_TRANSFORM});
    if (error)
        return error;

    error = cls_ecs_world_system_add(
        ui_world, "render", cls_render_system, 0.0f, 2,
        (const char *[]){CLS_COMP_RENDERABLE, CLS_COMP_TRANSFORM});
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

    const char *squares_id = "square";
    u32 squares_hash = 0;
    error = cls_xxhash32(&squares_hash, squares_id, strlen(squares_id), 0);
    if (error)
        return error;

    for (int i = 0; i < 100000; ++i) {
        cls_entity e = CLS_ENTITY_MAX;
        error = cls_preset_rect_spawn(
            &e, main_world, "",
            (vec2){cosf((float)i) * (i / 2.0f), sinf((float)i) * (i / 2.0f)},
            (vec2){5.0f, 5.0f}, 0.0f, 1.0f, (ivec4){255, 255, 255, 255},
            (vec2){1.0f, 1.0f}, (vec2){1.0f, 1.0f}, true);
        if (error)
            return error;

        error = cls_ecs_world_component_add(main_world, e, COMP_SQUARE_TAG,
                                            &(struct square_tag){});
        if (error)
            return error;
    }

    error = cls_preset_camera_ortho_spawn(
        NULL, ui_world, (vec3){0.0f, 0.0f, 0.0f}, 0.0f, (float)fb_size[0],
        (float)fb_size[1], 0.0f, 1.0f, -1.0f, 1.0f, true, 1, true);
    if (error)
        return error;

    error = cls_preset_label_spawn(NULL, ui_world, "fps", (vec2){10.0f, 30.0f},
                                   1.0f, "FPS: 0", 20, "human_sans-regular.otf",
                                   true, (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    error = cls_preset_label_spawn(
        NULL, ui_world, "entities", (vec2){10.0f, 50.0f}, 1.0f, "ENTITIES: 0",
        20, "human_sans-regular.otf", true, (ivec4){255, 255, 255, 255});
    if (error)
        return error;

    return cls_preset_label_spawn(
        NULL, ui_world, "ram", (vec2){10.0f, 70.0f}, 1.0f, "RAM: 0MB", 20,
        "human_sans-regular.otf", true, (ivec4){255, 255, 255, 255});

    return CLS_SUCCESS;
}

int main(void) {
    struct cls_gfx_api gl_api =
        (struct cls_gfx_api){.init = cls_gl_renderer_init,
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
    int error = cls_app_init(&app, &gl_api, (ivec2){WIN_WIDTH, WIN_HEIGHT},
                             WIN_TITLE, WIN_COLOR);
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
