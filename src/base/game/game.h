#ifndef GAME_H
#define GAME_H

#include "base/ecs/component/board.h"
#include "core/app/app.h"
#include "core/util/array.h"

struct game {
    struct array boards;
    enum player turn;
    enum player winner;
    unsigned int present;
    unsigned int timelines;
};

int game_init(struct game *game, struct app *app);
int game_run(struct app *app);

#endif // GAME_H
