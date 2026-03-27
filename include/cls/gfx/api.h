#ifndef CLS_API_H
#define CLS_API_H

#include <cglm/types.h>

typedef struct GLFWwindow GLFWwindow;
struct app;
struct array;
struct renderer_ctx;
struct shader;
struct shader_info;
struct cls_texture2d;
struct cls_texture2d_info;

struct gfx_api {
    int (*init)(ivec4 bg_color);
    int (*swap_buffers)(GLFWwindow *win);
    void (*on_resize)(int width, int height);
    int (*draw_frame)(struct app *app, struct array *transparent_cmds,
                      struct array *batches);
    int (*shader_init)(struct shader *s, const struct shader_info *info);
    void (*shader_destroy)(struct shader *s);
    int (*shader_use)(const struct shader *s);
    int (*texture2d_init)(struct cls_texture2d *tex,
                          struct cls_texture2d_info *info);
    void (*texture2d_destroy)(struct cls_texture2d *tex);
    int (*texture2d_use)(const struct cls_texture2d *tex);
};

#endif // CLS_API_H
