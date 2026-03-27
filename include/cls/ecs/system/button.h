#ifndef CLS_BUTTON_H
#define CLS_BUTTON_H

struct cls_app;
struct cls_ecs_world_query;

int button_system(struct cls_ecs_world_query *query, struct cls_app *app,
                  void *user_data);

#endif // CLS_BUTTON_H
