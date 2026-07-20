/**
 * @file cls/ecs/system/systems.c
 * @brief Default systems for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/system/systems.h
 */

#include <GLFW/glfw3.h>
#include <cglm/ivec4.h>
#include <cls/app/app.h>
#include <cls/app/assets.h>
#include <cls/app/window.h>
#include <cls/ecs/component/components.h>
#include <cls/ecs/ecs.h>
#include <cls/ecs/system/systems.h>
#include <cls/gfx/renderer.h>
#include <cls/io/font.h>
#include <cls/util/logger.h>

cls_error cls_button_system(struct cls_ecs_world_query *query,
                            struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_camera *cam_data = NULL;
    cls_entity cam = CLS_ENTITY_MAX;
    struct cls_ecs_world *world = NULL;
    cls_error error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    struct cls_ecs_world_query *cam_query = NULL;
    error = cls_ecs_world_query_create(
        &cam_query, world, 2,
        (const cls_component[]){CLS_COMP_CAMERA, CLS_COMP_CAMERA_ACTIVE});
    if (error)
        return error;

    void *cam_comps[2] = {NULL, NULL};
    bool found_camera = false;
    while (cls_ecs_world_query_next(&cam, cam_comps, cam_query) ==
               CLS_SUCCESS &&
           cam != CLS_ENTITY_MAX) {
        cam_data = cam_comps[0];
        found_camera = true;
        break;
    }

    cls_ecs_world_query_destroy(cam_query);

    if (!found_camera)
        return CLS_NULLPTR;

    cls_entity button = CLS_ENTITY_MAX;
    void *comps[3] = {NULL, NULL, NULL};
    while (cls_ecs_world_query_next(&button, comps, query) == CLS_SUCCESS &&
           button != CLS_ENTITY_MAX) {
        struct cls_button *button_comp = comps[1];
        struct cls_transform *tf = comps[2];

        float half_width = tf->scale[0] * 0.5f;
        float half_height = tf->scale[1] * 0.5f;
        float min_x = tf->pos[0] - half_width;
        float max_x = tf->pos[0] + half_width;
        float min_y = tf->pos[1] - half_height;
        float max_y = tf->pos[1] + half_height;

        vec2 cursor_pos = {0};
        error = cls_window_input_cursor_pos_get(cursor_pos, app->window);
        if (error)
            return error;

        ivec2 fb_size = {0};
        error = cls_window_fb_size_get(fb_size, app->window);
        if (error)
            return error;

        vec2 cursor_world = {0};
        cls_camera_screen_to_world(cursor_world, cam_data, cursor_pos, fb_size);

        bool inside = cursor_world[0] >= min_x && cursor_world[0] <= max_x &&
                      cursor_world[1] >= min_y && cursor_world[1] <= max_y;

        button_comp->hovering = inside;

        if (inside) {
            cls_window_input_mouse_pressed(&button_comp->pressed, app->window,
                                           GLFW_MOUSE_BUTTON_LEFT);
            cls_window_input_mouse_released(&button_comp->released, app->window,
                                            GLFW_MOUSE_BUTTON_LEFT);
            cls_window_input_mouse_down(&button_comp->down, app->window,
                                        GLFW_MOUSE_BUTTON_LEFT);
        }
    }

    return CLS_SUCCESS;
}

cls_error cls_camera_system(struct cls_ecs_world_query *query,
                            struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_ecs_world *world = NULL;
    cls_error error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    ivec2 fb_size = {0};
    error = cls_window_fb_size_get(fb_size, app->window);
    if (error)
        return error;
    struct cls_singleton_camera_active active = {0};

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[3] = {NULL, NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_camera *cam = comps[0];
        struct cls_transform *cam_tf = comps[2];
        if (!cam || !cam_tf)
            continue;

        if (fb_size[0] != (int)cam->ortho.right ||
            fb_size[1] != (int)cam->ortho.bottom)
            cls_camera_resize(cam, fb_size);

        if (cam->dirty || cam_tf->dirty) {
            error = cls_camera_update(cam, cam_tf);
            if (error)
                CLS_LOGGER_LOG_ERROR(CLS_LOGGER_ERROR, error, "%s",
                                     "Updating camera failed");
            cam->dirty = false;
            cam_tf->dirty = false;
        }

        active.cam = *cam;
        active.tf = *cam_tf;
        break;
    }

    return cls_ecs_world_singleton_add(world, CLS_SINGLETON_CAMERA_ACTIVE,
                                       &active, sizeof(active));
}

cls_error cls_label_render_system(struct cls_ecs_world_query *query,
                                  struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_renderer *rend = NULL;
    cls_error error = cls_window_renderer_get(&rend, app->window);
    if (error)
        return error;

    struct cls_ecs_world *world = NULL;
    error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    void *cam_ptr = NULL;
    error = cls_ecs_world_singleton_get(&cam_ptr, world,
                                        CLS_SINGLETON_CAMERA_ACTIVE);
    if (error)
        return CLS_SUCCESS;

    struct cls_singleton_camera_active *active = cam_ptr;
    if (!active)
        return CLS_SUCCESS;

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_label *l = comps[0];
        struct cls_transform *tf = comps[1];
        if (!l || !tf || !l->visible)
            continue;

        struct cls_font *f = NULL;
        error = cls_assets_font_get(&f, app->assets, l->font_id);
        if (error)
            return error;

        if (!f)
            return CLS_NULLPTR;

        float scale = (float)l->font_size / (float)f->pixel_size;
        float cursor_x = tf->pos[0];
        float cursor_y = tf->pos[1];
        size_t text_len = strlen(l->text);

        for (size_t i = 0; i < text_len; ++i) {
            u8 c = (u8)l->text[i];
            if (c < CLS_FONT_CHAR_START || c > CLS_FONT_CHAR_END)
                continue;

            const struct cls_glyph *glyph = &f->glyphs[c - CLS_FONT_CHAR_START];
            if (glyph->width == 0 || glyph->height == 0) {
                cursor_x += (float)glyph->advance * scale;
                continue;
            }

            float pos_x = cursor_x + (float)glyph->bearing_x * scale +
                          ((float)glyph->width * scale * 0.5f);
            float pos_y = cursor_y - (float)glyph->bearing_y * scale +
                          ((float)glyph->height * scale * 0.5f);

            struct cls_renderable ren = {
                .mesh_id = CLS_MESH_QUAD,
                .shader_id = CLS_SHADER_FONT,
                .texture_id = l->font_id,
                .uv_offset = {(float)glyph->atlas_x / (float)f->atlas_width,
                              (float)glyph->atlas_y / (float)f->atlas_height},
                .uv_scale = {(float)glyph->width / (float)f->atlas_width,
                             (float)glyph->height / (float)f->atlas_height},
                .visible = true,
                .opacity = 1.0f,
                .state = {.blending = true,
                          .blend_src = GL_SRC_ALPHA,
                          .blend_dest = GL_ONE_MINUS_SRC_ALPHA}};
            glm_ivec4_copy(l->tint, ren.tint);

            struct cls_transform glyph_tf = {
                .pos = {pos_x, pos_y, tf->pos[2]},
                .scale = {(float)glyph->width * scale,
                          (float)glyph->height * scale, 1.0f},
                .rot_axis = {0.0f, 0.0f, 1.0f},
                .rot_angle = 0.0f};

            vec3 delta = {0.0f};
            glm_vec3_sub(glyph_tf.pos, active->tf.pos, delta);
            float depth = glm_vec3_dot(active->cam.forward, delta);

            mat4 model = {{0.0f}};
            glm_mat4_identity(model);
            glm_translate(model, glyph_tf.pos);
            glm_scale(model, glyph_tf.scale);
            glm_mat4_identity(ren.mvp);
            glm_mat4_mul(ren.mvp, active->cam.projection, ren.mvp);
            glm_mat4_mul(ren.mvp, active->cam.view, ren.mvp);
            glm_mat4_mul(ren.mvp, model, ren.mvp);

            error = cls_renderer_cmd_push(rend, &ren, &glyph_tf, depth);
            if (error)
                continue;

            cursor_x += (float)glyph->advance * scale;
        }
    }

    return CLS_SUCCESS;
}

cls_error cls_render_system(struct cls_ecs_world_query *query,
                            struct cls_app *app) {
    if (!query || !app)
        return CLS_NULLPTR;

    struct cls_renderer *rend = NULL;
    cls_error error = cls_window_renderer_get(&rend, app->window);
    if (error)
        return error;

    struct cls_ecs_world *world = NULL;
    error = cls_ecs_world_query_world_get(&world, query);
    if (error)
        return error;

    error = cls_ecs_world_flush(world);
    if (error)
        return error;

    void *cam_ptr = NULL;
    error = cls_ecs_world_singleton_get(&cam_ptr, world,
                                        CLS_SINGLETON_CAMERA_ACTIVE);
    if (error)
        return CLS_SUCCESS;

    struct cls_singleton_camera_active *active = cam_ptr;
    if (!active)
        return CLS_SUCCESS;

    mat4 view_proj = {{0.0f}};
    glm_mat4_mul(active->cam.projection, active->cam.view, view_proj);

    cls_entity e = CLS_ENTITY_MAX;
    void *comps[2] = {NULL, NULL};
    while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
           e != CLS_ENTITY_MAX) {
        struct cls_renderable *ren = comps[0];
        struct cls_transform *tf = comps[1];
        if (!ren || !tf || !ren->visible)
            continue;

        vec3 delta = {0.0f};
        glm_vec3_sub(tf->pos, active->tf.pos, delta);
        float depth = glm_vec3_dot(active->cam.forward, delta);

        mat4 model = {{0.0f}};
        glm_mat4_identity(model);
        glm_translate(model, tf->pos);
        glm_rotate_at(model, tf->origin, tf->rot_angle, tf->rot_axis);
        glm_scale(model, tf->scale);

        glm_mat4_identity(ren->mvp);
        glm_mat4_mul(ren->mvp, model, ren->mvp);
        glm_mat4_mul(view_proj, model, ren->mvp);

        error = cls_renderer_cmd_push(rend, ren, tf, depth);
        if (error)
            continue;
    }

    return CLS_SUCCESS;
}
