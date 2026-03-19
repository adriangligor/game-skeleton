#include <X11/Xlib.h>
#include "platform.h"

static Display *g_display;
static Window   g_window;
static Atom     g_wm_delete_window;
static bool     g_running;

void platform_open_window(int width, int height, const char *title) {
    g_display = XOpenDisplay(NULL);
    int screen = DefaultScreen(g_display);

    Color c = PLATFORM_CLEAR_COLOR;
    unsigned long bg = ((unsigned long)c.r << 16)
                     | ((unsigned long)c.g << 8)
                     |  (unsigned long)c.b;

    g_window = XCreateSimpleWindow(
        g_display,
        RootWindow(g_display, screen),
        0, 0, width, height, 0,
        BlackPixel(g_display, screen),
        bg
    );

    XStoreName(g_display, g_window, title);
    XSelectInput(g_display, g_window, KeyPressMask | StructureNotifyMask);

    // intercept the close button instead of letting it kill the process
    g_wm_delete_window = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wm_delete_window, 1);

    XMapWindow(g_display, g_window);
    XFlush(g_display);

    g_running = true;
}

bool platform_running(void) {
    return g_running;
}

void platform_pump_events(void) {
    while (XPending(g_display)) {
        XEvent e;
        XNextEvent(g_display, &e);
        if (e.type == ClientMessage &&
            (Atom)e.xclient.data.l[0] == g_wm_delete_window) {
            g_running = false;
        }
    }
}
