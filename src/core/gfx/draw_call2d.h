#ifndef GFX_DRAW_CALL2D_H
#define GFX_DRAW_CALL2D_H

enum draw_call2d_render_type {
    MESH2D,
    SPRITE2D,
    DRAW_CALL2D_RENDER_TYPE_COUNT
};

struct draw_call2d {
    struct renderable2d *renderable;
    struct transform2d *transform;
    void *component;
    enum draw_call2d_render_type type;
};

int draw_call2d_init(struct draw_call2d *out, enum draw_call2d_render_type type,
                     void *component, struct renderable2d *renderable,
                     struct transform2d *transform);

#endif // GFX_DRAW_CALL2D_H
