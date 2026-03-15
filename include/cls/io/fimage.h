#ifndef CLS_FIMAGE_H
#define CLS_FIMAGE_H

#include <cls/util/types.h>

struct fimage {
    int width;
    int height;
    int channels;
    u8 *data;
};

int fimage_init(struct fimage *img, const char *path);
void fimage_destroy(struct fimage *img);
int fimage_save(const struct fimage *img, const char *path);

#endif // CLS_FIMAGE_H
