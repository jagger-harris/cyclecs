#include "core/gfx/renderer/renderer.h"
#include "core/gfx/api.h"
#include "core/gfx/renderer/batch.h"
#include "core/gfx/renderer/camera.h"
#include "core/gfx/renderer/cmd.h"
#include "core/gfx/renderer/plugins/plugins.h"
#include "core/util/logger.h"
#include "core/util/util.h"

static const struct renderer_plugin *plugins[] = {&RENDERER_PLUGIN_ECS,
                                                  &RENDERER_PLUGIN_UI};

#define NUM_PLUGINS ARRAY_LENGTH(plugins)

int renderer_init(struct renderer *out, struct gfx_api *api,
                  struct color bg_color) {
    if (!out)
        return CORE_NULLPTR;

    *out =
        (struct renderer){.api = api, .bg_color = bg_color, .plugin_count = 0};

    int error = array_init(&out->cmds, RENDERER_START_CMD_CAPACITY,
                           sizeof(struct renderer_cmd));
    if (error)
        goto cleanup;

    error = array_init(&out->batches, RENDERER_START_CMD_CAPACITY,
                       sizeof(struct renderer_batch));
    if (error)
        goto cleanup;

    error = table_init(&out->cameras, 8, GLOBALS_STR_ID_MAX,
                       sizeof(struct renderer_camera));
    if (error)
        goto cleanup;

    error = out->api->init();
    if (error)
        goto cleanup;

    return CORE_SUCCESS;

cleanup:
    renderer_destroy(out);
    return error;
}

void renderer_destroy(struct renderer *in) {
    if (!in)
        return;

    for (size_t i = 0; i < in->batches.length; ++i) {
        struct renderer_batch *batch = NULL;
        if (array_get((void **)&batch, &in->batches, i) == CORE_SUCCESS &&
            batch)
            array_destroy(&batch->cmds);
    }

    array_destroy(&in->batches);
    array_destroy(&in->cmds);
    table_destroy(&in->cameras);
}

int renderer_draw_frame(struct renderer *in, struct app *app) {
    if (!in || !app)
        return CORE_NULLPTR;

    if (in->active_camera && in->active_camera->update) {
        int error = renderer_camera_update(in->active_camera);
        if (error)
            LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                             "Updating renderer active camera failed");
        in->active_camera->update = false;
    }

    renderer_batch_clear_all(in);
    array_clear(&in->cmds);

    for (size_t i = 0; i < NUM_PLUGINS; ++i) {
        const struct renderer_plugin *plugin = plugins[i];
        if (plugin->create_cmds) {
            int error = plugin->create_cmds(in, app);
            if (error) {
                LOGGER_LOG_ERROR(LOGGER_ERROR, error,
                                 "Plugin '%s' failed to create draw commands",
                                 plugin->name);
            }
        }
    }

    int error = renderer_batch_build(&in->batches, &in->cmds);
    if (error)
        return error;

    return in->api->draw_frame(app, in->bg_color, &in->batches);
}

int renderer_swap_buffers(struct renderer *in, GLFWwindow *window) {
    if (!in)
        return CORE_NULLPTR;
    return in->api->swap_buffers(window);
}

int renderer_on_resize(struct renderer *in, int width, int height) {
    if (!in || !in->active_camera)
        return CORE_NULLPTR;

    renderer_camera_on_resize(in->active_camera, width, height);
    in->api->on_resize(width, height);
    in->active_camera->update = true;
    return CORE_SUCCESS;
}

int renderer_add_camera_ortho(struct renderer *in, const char *camera_id,
                              vec3 pos, float left, float right, float bottom,
                              float top, float zoom, float near_clip,
                              float far_clip) {
    return renderer_camera_add_ortho(&in->cameras, camera_id, pos, left, right,
                                     bottom, top, zoom, near_clip, far_clip);
}

int renderer_remove_camera(struct renderer *in, const char *camera_id) {
    return renderer_camera_remove(&in->cameras, camera_id);
}

int renderer_active_camera_get(struct renderer_camera **out,
                               struct renderer *in) {
    if (!out || !in)
        return CORE_NULLPTR;
    *out = in->active_camera;
    return CORE_SUCCESS;
}

int renderer_active_camera_set(struct renderer *in, const char *camera_id) {
    return renderer_camera_set_active(&in->cameras, &in->active_camera,
                                      camera_id);
}

int renderer_active_camera_pos_set(struct renderer *in, vec3 pos) {
    return renderer_camera_set_pos(in->active_camera, pos);
}

int renderer_active_camera_move(struct renderer *in, vec3 offset) {
    return renderer_camera_move(in->active_camera, offset);
}

int renderer_active_camera_update(struct renderer *in) {
    return renderer_camera_update(in->active_camera);
}

int renderer_screen_to_world(vec2 out, const struct renderer *in,
                             const vec2 screen_pos, const ivec2 viewport_size) {
    return renderer_camera_screen_to_world(out, in->active_camera, screen_pos,
                                           viewport_size);
}
