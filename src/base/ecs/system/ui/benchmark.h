#ifndef ECS_SYSTEM_UI_BENCHMARK_H
#define ECS_SYSTEM_UI_BENCHMARK_H

#include "base/game/game.h"
#include "core/app/app.h"
#include "core/app/window.h"
#include "core/ecs/component/camera.h"
#include "core/ecs/component/renderable/ui.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/error.h"
#include "core/util/logger.h"
#include <stdio.h>
#include <string.h>

int ui_benchmark_system(struct ecs_world_query *query, struct app *app) {
    if (!query || !query->world || !app)
        return CORE_NULLPTR;

    const char *fps_id = "fps";
    const char *entities_id = "entities";

    u32 fps_label_entity = U32_MAX;
    u32 entities_label_entity = U32_MAX;

    u32 entity = U32_MAX;
    while (ecs_world_query_next(&entity, query) == CORE_SUCCESS &&
           entity != U32_MAX) {
        struct ui_base *base = NULL;
        int error = ecs_world_query_get((void **)&base, query, ECS_COMP_UI_BASE,
                                        entity);
        if (error || !base)
            continue;

        if (strcmp(base->id, fps_id) == 0)
            fps_label_entity = entity;
        else if (strcmp(base->id, entities_id) == 0)
            entities_label_entity = entity;
    }

    if (fps_label_entity != U32_MAX)
        game_ui_label_entity_delete(query->world, fps_label_entity);

    if (entities_label_entity != U32_MAX)
        game_ui_label_entity_delete(query->world, entities_label_entity);

    float fps = 0;
    int error = window_timing_fps_avg_get(&fps, app->window);
    if (error)
        return error;

    char fps_text[ECS_COMPONENT_UI_TEXT_MAX] = {0};
    snprintf(fps_text, ECS_COMPONENT_UI_TEXT_MAX, "FPS: %i", (int)roundf(fps));

    game_ui_label_entity_new(
        query->world, app->assets, "fps", (vec2){10.0f, 25.0f}, 1.0f, fps_text,
        20, "human_sans-regular.otf", true, (ivec4){255, 255, 255, 255});

    const struct ecs_world *main_world = NULL;
    ecs_world_get(&main_world, app->ecs, GAME_WORLD_MAIN);
    if (!main_world)
        return CORE_SUCCESS;

    size_t entities = 0;
    error = ecs_world_entities_length_get(&entities, main_world);
    if (error)
        return error;

    char entities_text[ECS_COMPONENT_UI_TEXT_MAX] = {0};
    snprintf(entities_text, ECS_COMPONENT_UI_TEXT_MAX, "ENTITIES: %lu",
             entities);

    game_ui_label_entity_new(query->world, app->assets, "entities",
                             (vec2){10.0f, 45.0f}, 1.0f, entities_text, 20,
                             "human_sans-regular.otf", true,
                             (ivec4){255, 255, 255, 255});

    return CORE_SUCCESS;
}

#endif // ECS_SYSTEM_UI_BENCHMARK_H
