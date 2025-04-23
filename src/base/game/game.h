#ifndef GAME_H
#define GAME_H

#include "core/app/app.h"
#include "core/util/err.h"

err game_init(app *app);
err game_run(void);

#endif /* GAME_H */
