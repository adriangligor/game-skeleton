# game-skeleton

Minimal C game skeleton with a platform abstraction layer.

## Platform abstraction

`src/platform.h` declares the functions each backend must implement:

| Function | Purpose |
|---|---|
| `platform_open_window(w, h, title)` | Create and show the OS window |
| `platform_running()` | Returns `false` once the window is closed |
| `platform_pump_events()` | Drains the OS event queue and returns immediately |
| `platform_get_surface()` | Returns a pointer to the CPU pixel buffer |
| `platform_draw_surface()` | Uploads the pixel buffer to the GPU and presents |

The game loop in `src/main.c` is platform-independent:

```c
platform_open_window(640, 480, "Game");
while (platform_running()) {
    platform_pump_events();
    game_update();
    game_render();
}
```

`game_render` (in `src/game.c`) calls `platform_get_surface` to obtain an RGBA8 pixel buffer, writes into it, then calls `platform_draw_surface`.

### Backends

- **macOS** (`src/macos.m`) — Cocoa/Objective-C. Drains events with `nextEventMatchingMask:untilDate:distantPast`. Window close sets `g_running = false` via `NSWindowDelegate`.
- **Linux** (`src/linux.c`) — Xlib (X11). Drains events with `XPending`/`XNextEvent`. Window close is intercepted via the `WM_DELETE_WINDOW` atom to set `g_running = false`.
- **Windows** (`src/windows.c`) — Win32. Drains events with `PeekMessageA`/`DispatchMessageA`. Window close is handled by `DefWindowProcA` which calls `DestroyWindow` → triggers `WM_DESTROY` → sets `g_running = false`.

## Rendering pipeline

Each frame follows three steps:

1. **CPU compositing** — `game_render` writes RGBA8 pixels into the surface returned by `platform_get_surface`. This is ordinary memory; no GPU knowledge required.
2. **Upload** — `platform_draw_surface` copies the CPU buffer to a GPU texture (the "staging" step).
3. **Display** — the GPU samples the texture and outputs it to the screen via a minimal full-screen pass.

The surface pixel format is **RGBA8** (one byte per channel, R at the lowest address, stride = `width × 4` bytes) on all platforms, so assets can be written or `memcpy`'d in directly.

### Platform details

- **macOS** — `CAMetalLayer` with `MTLPixelFormatRGBA8Unorm`. Upload via `MTLTexture replaceRegion` (storage mode: `MTLStorageModeShared` on Apple Silicon, `MTLStorageModeManaged` on Intel). Display via a one-triangle render pass; a Metal fragment shader reads the staging texture by integer pixel coordinate (`tex.read(uint2(pos.xy))`). Shaders are compiled from a source string at startup — no `.metal` files or offline compilation step needed.
- **Linux** — X11 window with a GLX-compatible visual; OpenGL 3.3 core context via `glXCreateContextAttribsARB`. Upload via `glTexSubImage2D(GL_RGBA, ...)`. Display via a fullscreen triangle GLSL 3.30 shader (`gl_VertexID` trick, V-axis flipped to match the top-left CPU buffer origin). Links `-lX11 -lGL`; no extra libraries.
- **Windows** (planned) — Win32 window + Direct3D 11. Upload via `ID3D11DeviceContext::UpdateSubresource` to a `DXGI_FORMAT_R8G8B8A8_UNORM` texture. Display via a fullscreen pass HLSL shader.

## Build

macOS requires Xcode Command Line Tools:

```sh
xcode-select --install
```

Linux requires X11 and OpenGL development headers:

```sh
sudo apt install build-essential libx11-dev libgl-dev   # Debian/Ubuntu
sudo dnf install gcc make libX11-devel mesa-libGL-devel  # Fedora/RHEL
```

Windows requires Visual Studio (2019 16.8+ for C17 designated initializers) and GNU make (available via winget, Chocolatey, or Git for Windows). Build from a **Developer Command Prompt for VS** so `cl.exe` is on PATH:

```sh
winget install GnuWin32.Make
```

| Command | Effect |
|---|---|
| `make` | Build to `build/game` (or `build\game.exe` on Windows) |
| `make clean` | Remove `build/` |
| `make fmt` | Auto-format all source files in-place |

Run: `./build/game` (macOS/Linux) or `build\game.exe` (Windows)

## Dev tooling

`make fmt` formats all `.c`, `.h`, and `.m` source files in-place using [uncrustify](https://github.com/uncrustify/uncrustify). Style is defined in `.uncrustify.cfg` at the project root. Install uncrustify via your system package manager (`brew install uncrustify`, `apt install uncrustify`, `winget install uncrustify`) and ensure it is on your `PATH`.
