#include "core/gfx/renderer.h"
#include "core/gfx/camera.h"
#include "core/util/logger.h"
#include <cglm/cam.h>
#include <cglm/vec3.h>

err renderer_init(struct renderer *out, float aspect_ratio, const api_init init,
                  const api_swap_buffers swap, const api_on_resize resize,
                  const api_render_frame render_frame) {
    err status = CORE_SUCCESS;

    if (!out || !init || !swap || !resize || !render_frame) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = array_init(&out->opaque_draws, 64, sizeof(struct draw_call));
    if (status)
        goto err;

    status = array_init(&out->transparent_draws, 64, sizeof(struct draw_call));
    if (status)
        goto err;

    out->camera = (struct camera){.view = {{0.0F}},
                                  .projection = {{0.0F}},
                                  .pos = {0.0F},
                                  .rot = {0.0F},
                                  .fov = 90.0F,
                                  .near_clip = 0.1F,
                                  .far_clip = 100.0F,
                                  .aspect_ratio = aspect_ratio,
                                  .ortho_size = 1.0F,
                                  .orthographic = true,
                                  .active = true,
                                  .update = true};

    out->init = init;
    out->swap = swap;
    out->resize = resize;
    out->render_frame = render_frame;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Init renderer failed");
    return status;
}

void renderer_destroy(struct renderer *in) {
    if (!in)
        return;

    array_destroy(&in->opaque_draws);
    array_destroy(&in->transparent_draws);
}

err renderer_draw_call_add(struct renderer *in, struct draw_call *call) {
    err status = CORE_SUCCESS;

    if (!in || !call) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (call->mat->transparent) {
        status = array_push(&in->transparent_draws, call);
        if (status)
            goto err;
    } else {
        status = array_push(&in->opaque_draws, call);
        if (status)
            goto err;
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Adding draw to renderer queue failed");
    return status;
}

err renderer_use(const struct renderer *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = in->init();
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Using renderer failed");
    return status;
}

err renderer_swap_buffers(struct renderer *in, GLFWwindow *window) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    status = in->swap(window);
    if (status)
        goto err;

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Swapping buffers failed");
    return status;
}

err renderer_resize(struct renderer *in, int width, int height) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    in->resize(width, height);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Resizing renderer failed");
    return status;
}

err renderer_camera_update(struct renderer *in) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    struct camera *cam = &in->camera;
    vec3 forward = {0.0F};
    vec3 target = {0.0F};
    vec3 up = {0.0F, 1.0F, 0.0F};
    float pitch = glm_rad(cam->rot[0]);
    float yaw = glm_rad(cam->rot[1]);

    forward[0] = cosf(pitch) * sinf(yaw);
    forward[1] = sinf(pitch);
    forward[2] = cosf(pitch);

    glm_vec3_add(cam->pos, forward, target);
    glm_lookat(cam->pos, target, up, cam->view);

    if (cam->orthographic) {
        float top = cam->ortho_size;
        float bottom = -top;
        float right = top * cam->aspect_ratio;
        float left = -right;

        glm_ortho(left, right, bottom, top, cam->near_clip, cam->far_clip,
                  cam->projection);
    } else {
        glm_perspective(cam->fov, cam->aspect_ratio, cam->near_clip,
                        cam->far_clip, cam->projection);
    }

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Updating camera failed");
    return status;
}

err renderer_camera_set_pos(struct renderer *in, float pos_x, float pos_y,
                            float pos_z) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    in->camera.pos[0] = pos_x;
    in->camera.pos[1] = pos_y;
    in->camera.pos[2] = pos_z;

    in->camera.update = true;
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Update camera failed");
    return status;
}

err renderer_camera_move(struct renderer *in, float pos_x, float pos_y,
                         float pos_z) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    renderer_camera_set_pos(in, pos_x + in->camera.pos[0],
                            pos_y + in->camera.pos[1],
                            pos_z + in->camera.pos[2]);

    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Moving camera failed");
    return status;
}

err renderer_render_frame(struct renderer *in, struct assets *assets) {
    err status = CORE_SUCCESS;

    if (!in) {
        status = CORE_NULLPTR;
        goto err;
    }

    if (in->camera.update) {
        renderer_camera_update(in);
        in->camera.update = false;
    }

    status = in->render_frame(assets, &in->camera, &in->opaque_draws,
                              &in->transparent_draws);
    if (status)
        goto err;

    array_clear(&in->opaque_draws);
    array_clear(&in->transparent_draws);
    return status;

err:
    logger_log_err(LOGGER_ERR, status, "Clearing renderer failed");
    return status;
}
