#ifndef CLS_IMAGE_H
#define CLS_IMAGE_H

#include <cls/util/types.h>

struct image {
    int width;
    int height;
    int channels;
    u8 *data;
};

int image_init(struct image *img, const char *path);
void image_destroy(struct image *img);
int image_save(const struct image *img, const char *path);

#endif // CLS_IMAGE_H
