#include "platform.h"
#include "surface.h"
#include "game.h"

static Surface* init_graphics(int width, int height, const char *title) {
    platform_open_window(width, height, title);
    Surface *s = surface_create(width, height);

    return s;
}

int main(void) {
    Surface *s = init_graphics(640, 480, "Game");

    while (platform_running()) {
        platform_pump_events();

        game_update();

        game_render(s);
        platform_draw_surface(s);
    }

    surface_destroy(s);

    return 0;
}
