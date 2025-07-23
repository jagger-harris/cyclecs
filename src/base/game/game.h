#ifndef GAME_H
#define GAME_H

#include "core/app/app.h"
#include "core/util/err.h"

err game_init(struct app *app);
err game_run(struct app *app);

#endif // GAME_H
