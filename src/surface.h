#pragma once
#include <stddef.h>

typedef struct {
    unsigned char *pixels; // RGBA8, 4 bytes/pixel, stride = width*4
    int width;
    int height;
} Surface;

Surface *surface_create(int width, int height);
void     surface_destroy(Surface *s);
