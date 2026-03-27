#ifndef CLS_IMAGE_H
#define CLS_IMAGE_H

#include <cls/util/types.h>

struct cls_image {
    int width;
    int height;
    int channels;
    u8 *data;
};

int cls_image_init(struct cls_image *img, const char *path);
void cls_image_destroy(struct cls_image *img);
int cls_image_save(const struct cls_image *img, const char *path);

#endif // CLS_IMAGE_H
