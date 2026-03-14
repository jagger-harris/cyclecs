#ifndef ECS_SYSTEM_UI_BUTTON_H
#define ECS_SYSTEM_UI_BUTTON_H

struct app;
struct ecs_world_query;

int ui_button_system(struct ecs_world_query *query, struct app *app,
                     void *user_data);

#endif // ECS_SYSTEM_UI_BUTTON_H
