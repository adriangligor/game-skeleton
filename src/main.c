#include "platform.h"
#include "game.h"

int main(void) {
    platform_open_window(640, 480, "Game");
    while (platform_running()) {
        platform_pump_events();
        game_update();
        game_render();
    }
    return 0;
}
