#ifndef CLS_BUTTON_H
#define CLS_BUTTON_H

struct app;
struct ecs_world_query;

int button_system(struct ecs_world_query *query, struct app *app,
                  void *user_data);

#endif // CLS_BUTTON_H
