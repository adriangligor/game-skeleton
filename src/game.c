#include "game.h"
#include "input.h"
#include "platform.h"

void game_update(void) {
    Event e;

    while (input_next_event(&e)) {
        switch (e.type) {
            case EVENT_SYSTEM_QUIT:
                platform_quit();
                break;
            case EVENT_KEY_DOWN:
                break;
            case EVENT_KEY_UP:
                break;
            default:
                break;
        }
    }
}

void game_render(Surface *s) {
    // Checkerboard test pattern
    for (int y = 0; y < s->height; y++) {
        for (int x = 0; x < s->width; x++) {
            unsigned char *p = s->pixels + (y * s->width + x) * 4;
            int on = ((x / 32) ^ (y / 32)) & 1;
            p[0] = on ? 255 : 40;  // R
            p[1] = on ? 255 : 60;  // G
            p[2] = on ? 255 : 80;  // B
            p[3] = 255;            // A
        }
    }

}
