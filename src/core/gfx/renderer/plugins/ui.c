#include "core/app/ui.h"
#include "core/app/app.h"
#include "core/app/assets.h"
#include "core/gfx/renderer/cmd.h"
#include "core/gfx/renderer/renderer.h"
#include "core/io/ffont.h"
#include "core/util/logger.h"
#include "core/util/util.h"

typedef int (*create_ui_widget_cmds_fn)(struct array *out,
                                        struct ui_widget *widget,
                                        struct app *app);

struct ui_widget_handler {
    enum ui_widget_type type;
    create_ui_widget_cmds_fn create_cmds;
};

static int create_ui_widget_label_cmds(struct array *cmds,
                                       struct ui_widget *widget,
                                       struct app *app) {
    struct ui_widget_label *label = NULL;
    int error =
        array_get((void **)&label, &app->window.ui.labels, widget->data_index);
    if (error)
        return error;

    struct ffont *font = NULL;
    error = assets_font_get(&font, &app->assets, label->font_id);
    if (error)
        return error;

    if (!font) {
        LOGGER_LOG(LOGGER_ERROR, "%s",
                   "No font found for rendering widget label");
        return CORE_FAILURE;
    }

    error = array_init(cmds, strlen(label->text), sizeof(struct renderer_cmd));
    if (error)
        return error;

    const float base_font_size = 64.0f;
    float scale = (float)label->font_size / base_font_size;
    float cursor_x = widget->transform.pos[0];
    float cursor_y = widget->transform.pos[1];
    size_t text_len = strlen(label->text);

    for (size_t i = 0; i < text_len; ++i) {
        unsigned char c = (unsigned char)label->text[i];

        if (c < FFONT_CHAR_START || c > FFONT_CHAR_END)
            continue;

        struct fglyph *glyph = &font->glyphs[c - FFONT_CHAR_START];

        if (glyph->width == 0 || glyph->height == 0) {
            cursor_x += (float)glyph->advance * scale;
            continue;
        }

        struct renderer_cmd cmd = {0};

        float pos_x = cursor_x + (float)glyph->bearing_x * scale +
                      ((float)glyph->width * scale / 2.0f);
        float pos_y = cursor_y - (float)glyph->bearing_y * scale +
                      ((float)glyph->height * scale / 2.0f);

        int ret = snprintf(cmd.mesh_id, GLOBALS_PATH_MAX, "%s", "quad");
        if (ret < 0) {
            error = CORE_FAILURE;
            goto cleanup;
        }

        ret = snprintf(cmd.shader_id, GLOBALS_PATH_MAX, "%s", "font");
        if (ret < 0) {
            error = CORE_FAILURE;
            goto cleanup;
        }

        ret = snprintf(cmd.texture_id, GLOBALS_PATH_MAX, "%s", label->font_id);
        if (ret < 0) {
            error = CORE_FAILURE;
            goto cleanup;
        }

        cmd.transform.pos[0] = pos_x;
        cmd.transform.pos[1] = pos_y;
        cmd.transform.pos[2] = widget->transform.pos[2];
        cmd.transform.scale[0] = (float)glyph->width * scale;
        cmd.transform.scale[1] = (float)glyph->height * scale;
        cmd.transform.scale[2] = 1.0f;

        cmd.uv_offset[0] = (float)glyph->atlas_x / (float)font->atlas_width;
        cmd.uv_offset[1] = (float)glyph->atlas_y / (float)font->atlas_height;
        cmd.uv_scale[0] = (float)glyph->width / (float)font->atlas_width;
        cmd.uv_scale[1] = (float)glyph->height / (float)font->atlas_height;

        memcpy(&cmd.tint, &widget->tint, sizeof(struct color));
        cmd.opacity = widget->opacity;
        cmd.transparent = true;
        cmd.is_in_world = false;

        error = array_push(cmds, &cmd);
        if (error) {
            array_destroy(cmds);
            return error;
        }

        cursor_x += (float)glyph->advance * scale;
    }

    return CORE_SUCCESS;

cleanup:
    array_destroy(cmds);
    return error;
}

static const struct ui_widget_handler WIDGET_HANDLERS[] = {
    {UI_WIDGET_LABEL, create_ui_widget_label_cmds},
};

#define WIDGET_HANDLERS_LENGTH ARRAY_LENGTH(WIDGET_HANDLERS)

static int process_widget(struct renderer *renderer, struct ui_widget *widget,
                          struct app *app,
                          const struct ui_widget_handler *handler) {
    struct array cmds = {0};

    int error = handler->create_cmds(&cmds, widget, app);
    if (error)
        return error;

    error = array_concat(&renderer->cmds, &cmds);
    if (error) {
        array_destroy(&cmds);
        LOGGER_LOG_ERROR(LOGGER_ERROR, error, "%s",
                         "Adding ui widget draw cmd failed");
        return error;
    }

    array_destroy(&cmds);
    return CORE_SUCCESS;
}

static int ui_plugin_create_cmds(struct renderer *renderer, struct app *app) {
    if (!renderer || !app)
        return CORE_NULLPTR;

    struct table *widgets = &app->window.ui.widgets;
    struct table_iterator iter;
    int error = table_iterator_init(&iter, widgets);
    if (error)
        return error;

    while (table_iterator_next(&iter)) {
        struct ui_widget *widget = iter.value;

        if (!widget->visible || widget->opacity == 0.0f)
            continue;

        for (size_t j = 0; j < WIDGET_HANDLERS_LENGTH; ++j) {
            if (WIDGET_HANDLERS[j].type == widget->type) {
                process_widget(renderer, widget, app, &WIDGET_HANDLERS[j]);
                break;
            }
        }
    }

    return CORE_SUCCESS;
}

const struct renderer_plugin RENDERER_PLUGIN_UI = {
    .name = "ui", .create_cmds = ui_plugin_create_cmds};
