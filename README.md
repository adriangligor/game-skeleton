# game-skeleton

Minimal C game skeleton with a platform abstraction layer.

## Platform abstraction

`src/platform.h` declares three functions each backend must implement:

| Function | Purpose |
|---|---|
| `platform_open_window(w, h, title)` | Create and show the OS window |
| `platform_running()` | Returns `false` once the window is closed |
| `platform_pump_events()` | Drains the OS event queue and returns immediately |

The game loop in `src/main.c` is platform-independent:

```c
platform_open_window(640, 480, "Game");
while (platform_running()) {
    platform_pump_events();
    // game_update(); game_render();
}
```

### Backends

- **macOS** (`src/macos.m`) — Cocoa/Objective-C. Drains events with `nextEventMatchingMask:untilDate:distantPast`. Window close sets `g_running = false` via `NSWindowDelegate`.
- **Linux** (`src/linux.c`) — Xlib (X11). Drains events with `XPending`/`XNextEvent`. Window close is intercepted via the `WM_DELETE_WINDOW` atom to set `g_running = false`.

## Build

macOS requires Xcode Command Line Tools:

```sh
xcode-select --install
```

Linux requires the X11 development headers:

```sh
sudo apt install build-essential libx11-dev   # Debian/Ubuntu
sudo dnf install gcc make libX11-devel        # Fedora/RHEL
```

| Command | Effect |
|---|---|
| `make` | Build to `build/game` |
| `make clean` | Remove `build/` |

Run: `./build/game`
