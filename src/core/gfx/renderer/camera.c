#include "core/gfx/renderer/camera.h"
#include "core/util/error.h"
#include "core/util/globals.h"
#include "core/util/table.h"
#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>
#include <cglm/types.h>
#include <cglm/util.h>
#include <cglm/vec3.h>
#include <math.h>
#include <stdio.h>

int renderer_camera_add_ortho(struct table *cameras, const char *camera_id,
                              vec3 pos, float left, float right, float bottom,
                              float top, float zoom, float near_clip,
                              float far_clip) {
    if (!cameras || !camera_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", camera_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct renderer_camera new_camera = {.type = CAMERA_ORTHO,
                                         .pos = {pos[0], pos[1], pos[2]},
                                         .ortho.left = left,
                                         .ortho.right = right,
                                         .ortho.bottom = bottom,
                                         .ortho.top = top,
                                         .zoom = zoom,
                                         .near_clip = near_clip,
                                         .far_clip = far_clip,
                                         .active = false,
                                         .update = true};

    glm_mat4_identity(new_camera.view);
    glm_mat4_identity(new_camera.projection);
    return table_insert(cameras, id, &new_camera);
}

int renderer_camera_remove(struct table *cameras, const char *camera_id) {
    if (!cameras || !camera_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", camera_id);
    if (ret < 0)
        return CORE_FAILURE;

    return table_remove(NULL, cameras, id);
}

int renderer_camera_set_active(struct table *cameras,
                               struct renderer_camera **active_camera,
                               const char *camera_id) {
    if (!cameras || !active_camera || !camera_id)
        return CORE_NULLPTR;

    char id[GLOBALS_STR_ID_MAX] = {0};
    int ret = snprintf(id, GLOBALS_STR_ID_MAX, "%s", camera_id);
    if (ret < 0)
        return CORE_FAILURE;

    struct renderer_camera *new_active = NULL;
    int error = table_find((void **)&new_active, cameras, &id);
    if (error)
        return error;

    struct table_iterator iter = {0};
    error = table_iterator_init(&iter, cameras);
    if (error)
        return error;

    while (table_iterator_next(&iter)) {
        struct renderer_camera *cam = iter.value;
        cam->active = false;
    }

    new_active->active = true;
    *active_camera = new_active;
    return CORE_SUCCESS;
}

int renderer_camera_update(struct renderer_camera *in) {
    if (!in)
        return CORE_NULLPTR;

    switch (in->type) {
    case CAMERA_PERSP: {
        glm_perspective(in->persp.fov, in->persp.aspect_ratio, in->near_clip,
                        in->far_clip, in->projection);

        vec3 forward = {0.0f};
        vec3 target = {0.0f};
        vec3 up = {0.0f, 1.0f, 0.0f};
        float pitch = glm_rad(in->rot_axis[0]);
        float yaw = glm_rad(in->rot_axis[1]);

        forward[0] = cosf(pitch) * cosf(yaw);
        forward[1] = sinf(pitch);
        forward[2] = cosf(pitch) * sinf(yaw);

        glm_vec3_norm(forward);
        glm_vec3_add(in->pos, forward, target);
        glm_lookat(in->pos, target, up, in->view);
        break;
    }
    case CAMERA_ORTHO: {
        float zoom = in->zoom;
        if (zoom <= 0.0f)
            zoom = 0.0001f;

        float width = (in->ortho.right - in->ortho.left) / zoom;
        float height = (in->ortho.bottom - in->ortho.top) / zoom;

        float center_x = (in->ortho.right + in->ortho.left) * 0.5f;
        float center_y = (in->ortho.top + in->ortho.bottom) * 0.5f;

        float left = center_x - width * 0.5f;
        float right = center_x + width * 0.5f;
        float bottom = center_y + height * 0.5f;
        float top = center_y - height * 0.5f;

        glm_ortho(left, right, bottom, top, in->near_clip, in->far_clip,
                  in->projection);

        mat4 view = {{0.0f}};
        glm_mat4_identity(view);
        glm_translate(view, (vec3){-in->pos[0], -in->pos[1], -in->pos[2]});
        glm_mat4_copy(view, in->view);
        break;
    }
    default:
        return CORE_INVALID_ARG;
    }

    return CORE_SUCCESS;
}

void renderer_camera_on_resize(struct renderer_camera *camera, int width,
                               int height) {
    if (!camera)
        return;

    switch (camera->type) {
    case CAMERA_PERSP:
        camera->persp.aspect_ratio = (float)width / (float)height;
        break;
    case CAMERA_ORTHO:
        camera->ortho.left = 0.0f;
        camera->ortho.right = (float)width;
        camera->ortho.top = 0.0f;
        camera->ortho.bottom = (float)height;
        break;
    default:
        break;
    }
}

int renderer_camera_set_pos(struct renderer_camera *camera, vec3 pos) {
    if (!camera)
        return CORE_NULLPTR;

    camera->pos[0] = pos[0];
    camera->pos[1] = pos[1];
    camera->pos[2] = pos[2];
    camera->update = true;
    return CORE_SUCCESS;
}

int renderer_camera_move(struct renderer_camera *camera, vec3 offset) {
    if (!camera)
        return CORE_NULLPTR;

    vec3 new_pos = {camera->pos[0] + offset[0], camera->pos[1] + offset[1],
                    camera->pos[2] + offset[2]};

    return renderer_camera_set_pos(camera, new_pos);
}

int renderer_camera_screen_to_world(vec2 out, struct renderer_camera *camera,
                                    const vec2 screen_pos,
                                    const ivec2 viewport_size) {
    if (!camera)
        return CORE_NULLPTR;

    if (viewport_size[0] <= 0 || viewport_size[1] <= 0)
        return CORE_INVALID_ARG;

    float ndc_x = (2.0f * screen_pos[0]) / (float)viewport_size[0] - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_pos[1]) / (float)viewport_size[1];
    vec4 ndc = {ndc_x, ndc_y, -1.0f, 1.0f};

    mat4 view_projection = {{0}};
    mat4 inverse_view_projection = {{0}};
    vec4 world = {0};
    glm_mat4_mul(camera->projection, camera->view, view_projection);
    glm_mat4_inv(view_projection, inverse_view_projection);
    glm_mat4_mulv(inverse_view_projection, ndc, world);

    out[0] = world[0] / world[3];
    out[1] = world[1] / world[3];
    return CORE_SUCCESS;
}
