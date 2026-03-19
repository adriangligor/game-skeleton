#pragma once
#include <stdbool.h>

typedef struct { unsigned char r, g, b; } Color;

#define PLATFORM_CLEAR_COLOR ((Color){ 51, 77, 102 })

void platform_open_window(int width, int height, const char *title);
bool platform_running(void);
void platform_pump_events(void);
