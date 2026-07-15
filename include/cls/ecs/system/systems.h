#ifndef CLS_SYSTEMS_H
#define CLS_SYSTEMS_H

#include <cls/util/error.h>

struct cls_app;
struct cls_ecs_world_query;

cls_error cls_button_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);
cls_error cls_camera_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);
cls_error cls_label_render_system(struct cls_ecs_world_query *query,
                                  struct cls_app *app);
cls_error cls_render_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);

#endif // CLS_SYSTEMS_H
