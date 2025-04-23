#ifndef APP_H
#define APP_H

#include "core/app/assets.h"
#include "core/util/arena.h"
#include "core/util/err.h"

typedef struct app app;
typedef int (*game_new)(app *);
typedef int (*game_update)(void);

err app_new(app **out, arena *mem, int width, int height, const char *title);
err app_delete(app *in);
err app_run(app *in, game_new game, game_update update);
err app_get_assets(assets **out, app *in);

#endif /* APP_H */
