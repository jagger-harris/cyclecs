#ifndef APP_TIME_H
#define APP_TIME_H

struct app_time {
    float delta_time;
    float total;
    double last_frame;

    int frame_count;
    float fps;
    float fps_avg;
    float fps_timer;
};

int app_time_init(struct app_time *out);
int app_time_update(struct app_time *in);
int app_time_get_fps(float *out, const struct app_time *in);
int app_time_get_fps_avg(float *out, const struct app_time *in);

#endif // APP_TIME_H
