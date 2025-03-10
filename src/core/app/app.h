#ifndef APP_H
#define APP_H

#include "../util/arena.h"

typedef struct app app;

int app_new(app **out, arena *in, int width, int height, const char *title);
int app_delete(app *in);
int app_run(app *in);

#endif /* APP_H */
