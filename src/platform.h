#pragma once
#include <stdbool.h>

typedef struct { unsigned char r, g, b; } Color;

#define PLATFORM_CLEAR_COLOR ((Color){ 51, 77, 102 })

typedef struct {
    unsigned char *pixels; // RGBA8, 4 bytes/pixel, stride = width*4
    int width;
    int height;
} Surface;

void     platform_open_window(int width, int height, const char *title);
bool     platform_running(void);
void     platform_pump_events(void);
Surface *platform_get_surface(void);  // CPU backbuffer; valid until platform_draw_surface
void     platform_draw_surface(void); // upload pixels to GPU and present
