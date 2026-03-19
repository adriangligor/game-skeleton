#pragma once
#include <stdbool.h>

void platform_open_window(int width, int height, const char *title);
bool platform_running(void);
void platform_pump_events(void);
