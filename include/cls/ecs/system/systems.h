#ifndef CLS_SYSTEMS_H
#define CLS_SYSTEMS_H

struct cls_app;
struct cls_ecs_world_query;

int cls_button_system(struct cls_ecs_world_query *query, struct cls_app *app);
int cls_camera_system(struct cls_ecs_world_query *query, struct cls_app *app);
int cls_label_render_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);
int cls_render_system(struct cls_ecs_world_query *query, struct cls_app *app);

#endif // CLS_SYSTEMS_H
