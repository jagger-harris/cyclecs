#ifndef ECS_CTX_H
#define ECS_CTX_H

typedef struct assets assets;
typedef struct renderer renderer;
typedef struct window window;

typedef struct ecs_ctx ecs_ctx;
struct ecs_ctx {
    assets *assets;
    renderer *renderer;
    window *window;
};

#endif /* ECS_CXT_H */
