#ifndef GFX_API_H
#define GFX_API_H

typedef struct GLFWwindow GLFWwindow;
struct app;
struct array;
struct color;
struct renderer_ctx;
struct shader;
struct shader_info;
struct texture2d;
struct texture2d_info;

enum api_type { GL };

struct gfx_api {
    enum api_type type;
    int (*init)(void);
    int (*swap_buffers)(GLFWwindow *window);
    void (*on_resize)(int width, int height);
    int (*draw_frame)(struct app *app, struct color bg_color,
                      struct array *render_batches);
    int (*shader_init)(struct shader *out, const struct shader_info *info);
    void (*shader_destroy)(struct shader *in);
    int (*shader_use)(struct shader *in);
    int (*texture2d_init)(struct texture2d *out, struct texture2d_info *info);
    void (*texture2d_destroy)(struct texture2d *in);
    int (*texture2d_use)(struct texture2d *in);
};

#endif // GFX_API_H
