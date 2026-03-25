#include "surface.h"
#include <stdlib.h>

Surface *surface_create(int width, int height) {
    Surface *s = malloc(sizeof(Surface));
    s->pixels = malloc((size_t)(width * height * 4));
    s->width = width;
    s->height = height;

    return s;
}

void surface_destroy(Surface *s) {
    free(s->pixels);
    free(s);
}
