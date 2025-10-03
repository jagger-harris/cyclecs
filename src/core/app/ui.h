#ifndef APP_UI_H
#define APP_UI_H

#include "core/gfx/color.h"
#include "core/gfx/transform.h"
#include "core/util/array.h"
#include "core/util/globals.h"
#include "core/util/table.h"
#include <stdbool.h>

#define UI_WIDGET_TEXT_MAX 128

typedef int (*on_click_fn)(void);
typedef int (*on_hover_fn)(void);

struct input;

struct ui {
    struct table widgets;
    struct array labels;
};

enum ui_widget_type {
    UI_WIDGET_TEXT_BUTTON = 0,
    UI_WIDGET_LABEL,
    UI_WIDGET_PANEL,
    UI_PROGRESS_BAR
};

struct ui_widget {
    char texture_id[GLOBALS_PATH_MAX];
    struct transform transform;
    struct color tint;
    enum ui_widget_type type;
    struct ui_widget *parent;
    struct array *children;
    size_t data_index;
    on_click_fn on_click;
    on_click_fn on_hover;
    float opacity;
    bool visible;
};

struct ui_widget_button {
    char font_id[GLOBALS_PATH_MAX];
    char text[UI_WIDGET_TEXT_MAX];
    int font_size;
    bool pressed;
};

struct ui_widget_label {
    char font_id[GLOBALS_PATH_MAX];
    char text[UI_WIDGET_TEXT_MAX];
    int font_size;
};

struct ui_widget_progress_bar {
    float value;
    float max_value;
};

int ui_init(struct ui *out);
void ui_destroy(struct ui *in);
int ui_handle_input(struct ui *in, struct input *input);
int ui_widget_add_label(struct ui *in, const char *widget_id, const char *text,
                        int font_size, const char *font_id, bool visible,
                        struct transform transform, struct color tint,
                        const on_click_fn on_click, const on_hover_fn on_hover);

#endif // APP_UI_H
