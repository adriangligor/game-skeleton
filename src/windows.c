#include <windows.h>
#include "platform.h"

static HWND g_hwnd;
static bool g_running;

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Color c = PLATFORM_CLEAR_COLOR;
            HBRUSH brush = CreateSolidBrush(RGB(c.r, c.g, c.b));
            FillRect(hdc, &ps.rcPaint, brush);
            DeleteObject(brush);
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

void platform_open_window(int width, int height, const char *title) {
    SetProcessDPIAware();

    HINSTANCE hInstance = GetModuleHandleA(NULL);

    WNDCLASSEXA wc = {
        .cbSize        = sizeof(WNDCLASSEXA),
        .lpfnWndProc   = window_proc,
        .hInstance     = hInstance,
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = "GameWindow",
    };
    RegisterClassExA(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowExA(
        0,
        "GameWindow",
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hwnd, SW_SHOW);
    g_running = true;
}

bool platform_running(void) {
    return g_running;
}

void platform_pump_events(void) {
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
