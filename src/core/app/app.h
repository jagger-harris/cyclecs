#ifndef APP_H
#define APP_H

typedef struct app app;

int app_new(app *out, int width, int height, const char *title);
int app_delete(app *in);
int app_run(app *in);

#endif /* APP_H */
