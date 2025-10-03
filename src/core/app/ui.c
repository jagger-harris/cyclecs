#include "core/app/ui.h"
#include "core/app/input.h"
#include "core/gfx/color.h"
#include "core/gfx/transform.h"
#include "core/util/error.h"
#include <GLFW/glfw3.h>
#include <stdalign.h>
#include <stdio.h>

#define UI_START_CAPACITY 8

int ui_init(struct ui *out) {
    if (!out)
        return CORE_NULLPTR;

    int error = table_init(&out->widgets, UI_START_CAPACITY, GLOBALS_STR_ID_MAX,
                           sizeof(struct ui_widget));
    if (error)
        return error;

    error = array_init(&out->labels, UI_START_CAPACITY,
                       sizeof(struct ui_widget_label));
    if (error)
        return error;

    return CORE_SUCCESS;
}

void ui_destroy(struct ui *in) {
    if (!in)
        return;

    table_destroy(&in->widgets);
    array_destroy(&in->labels);
}

int ui_handle_input(struct ui *in, struct input *input) {
    if (!in || !input)
        return CORE_NULLPTR;

    struct table_iterator iter;
    int error = table_iterator_init(&iter, &in->widgets);
    if (error)
        return error;

    bool mouse_released = false;
    while (table_iterator_next(&iter)) {
        struct ui_widget *widget = iter.value;

        error = input_mouse_released(&mouse_released, input,
                                     GLFW_MOUSE_BUTTON_LEFT);
        if (error)
            continue;

        if (mouse_released) {
            if (widget->on_click)
                widget->on_click();
        }

        if (widget->on_hover)
            widget->on_hover();
    }

    return CORE_SUCCESS;
}

int ui_widget_add_label(struct ui *in, const char *widget_id, const char *text,
                        int font_size, const char *font_id, bool visible,
                        struct transform transform, struct color tint,
                        const on_click_fn on_click,
                        const on_hover_fn on_hover) {
    if (!in || !text)
        return CORE_NULLPTR;

    struct ui_widget_label label = {.font_size = font_size};
    int ret = snprintf(label.text, UI_WIDGET_TEXT_MAX, "%s", text);
    if (ret < 0)
        return CORE_FAILURE;

    ret = snprintf(label.font_id, UI_WIDGET_TEXT_MAX, "%s", font_id);
    if (ret < 0)
        return CORE_FAILURE;

    int error = array_push(&in->labels, &label);
    if (error)
        return error;

    struct ui_widget widget = {.type = UI_WIDGET_LABEL,
                               .transform = transform,
                               .data_index = in->labels.length - 1,
                               .tint = tint,
                               .on_click = on_click,
                               .on_hover = on_hover,
                               .opacity = 1.0f,
                               .visible = visible};

    char id[GLOBALS_STR_ID_MAX] = {0};
    ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", widget_id);
    if (ret < 0)
        return CORE_FAILURE;

    error = table_insert(&in->widgets, id, &widget);
    if (error)
        return error;

    return CORE_SUCCESS;
}
