#include "core/app/app_time.h"
#include "core/util/error.h"
#include <GLFW/glfw3.h>

int app_time_init(struct app_time *out) {
    if (!out)
        return CORE_NULLPTR;

    out->delta_time = 0.0F;
    out->total = 0.0F;
    out->last_frame = glfwGetTime();

    out->frame_count = 0;
    out->fps = 0.0F;
    out->fps_avg = 0.0F;
    out->fps_timer = 0.0F;
    return CORE_SUCCESS;
}

int app_time_update(struct app_time *in) {
    if (!in)
        return CORE_NULLPTR;

    double now = glfwGetTime();
    in->delta_time = (float)(now - in->last_frame);
    in->total += in->delta_time;
    in->last_frame = now;

    if (in->delta_time > 0.0F)
        in->fps = 1.0F / in->delta_time;

    in->frame_count++;
    in->fps_timer += in->delta_time;

    if (in->fps_timer >= 1.0f) {
        in->fps_avg = (float)in->frame_count / in->fps_timer;
        in->frame_count = 0;
        in->fps_timer = 0.0F;
    }

    return CORE_SUCCESS;
}
