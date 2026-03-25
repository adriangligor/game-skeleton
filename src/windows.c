#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <string.h>
#include "platform.h"

static HWND                      g_hwnd;
static bool                      g_running;
static ID3D11Device             *g_device;
static ID3D11DeviceContext      *g_ctx;
static IDXGISwapChain           *g_swap_chain;
static ID3D11RenderTargetView   *g_rtv;
static ID3D11Texture2D          *g_texture;
static ID3D11ShaderResourceView *g_srv;
static ID3D11VertexShader       *g_vs;
static ID3D11PixelShader        *g_ps;

// Shaders compiled at runtime — no .hlsl files or fxc needed.
// Vertex shader: fullscreen triangle from SV_VertexID, no vertex buffer.
// Pixel shader: reads the staging texture at integer pixel coordinates,
// matching the CPU buffer's top-left origin (no UV flip needed in D3D11).
static const char *k_shader_src =
    "Texture2D<float4> tex : register(t0);\n"
    "float4 vs_main(uint vid : SV_VertexID) : SV_Position {\n"
    "    float2 v[3] = { float2(-1,1), float2(3,1), float2(-1,-3) };\n"
    "    return float4(v[vid], 0, 1);\n"
    "}\n"
    "float4 ps_main(float4 pos : SV_Position) : SV_Target {\n"
    "    return tex.Load(int3((int2)pos.xy, 0));\n"
    "}\n";

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            ValidateRect(hwnd, NULL);
            return 0;
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

    // Create D3D11 device and swap chain together
    DXGI_SWAP_CHAIN_DESC scd = {
        .BufferCount              = 1,
        .BufferDesc.Width         = (UINT)width,
        .BufferDesc.Height        = (UINT)height,
        .BufferDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BufferUsage              = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .OutputWindow             = g_hwnd,
        .SampleDesc.Count         = 1,
        .Windowed                 = TRUE,
        .SwapEffect               = DXGI_SWAP_EFFECT_DISCARD,
    };
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        &feature_level, 1, D3D11_SDK_VERSION,
        &scd, &g_swap_chain, &g_device, NULL, &g_ctx
    );

    // Render target view from the swap chain back buffer
    ID3D11Texture2D *back_buffer;
    IDXGISwapChain_GetBuffer(g_swap_chain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer);
    ID3D11Device_CreateRenderTargetView(g_device, (ID3D11Resource *)back_buffer, NULL, &g_rtv);
    ID3D11Texture2D_Release(back_buffer);

    // Staging texture for CPU→GPU upload: matches the CPU surface format exactly
    D3D11_TEXTURE2D_DESC td = {
        .Width            = (UINT)width,
        .Height           = (UINT)height,
        .MipLevels        = 1,
        .ArraySize        = 1,
        .Format           = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc.Count = 1,
        .Usage            = D3D11_USAGE_DEFAULT,
        .BindFlags        = D3D11_BIND_SHADER_RESOURCE,
    };
    ID3D11Device_CreateTexture2D(g_device, &td, NULL, &g_texture);
    ID3D11Device_CreateShaderResourceView(g_device, (ID3D11Resource *)g_texture, NULL, &g_srv);

    // Compile shaders from source at startup
    ID3DBlob *vs_blob, *ps_blob;
    SIZE_T src_len = strlen(k_shader_src);
    D3DCompile(k_shader_src, src_len, NULL, NULL, NULL,
               "vs_main", "vs_5_0", 0, 0, &vs_blob, NULL);
    D3DCompile(k_shader_src, src_len, NULL, NULL, NULL,
               "ps_main", "ps_5_0", 0, 0, &ps_blob, NULL);
    ID3D11Device_CreateVertexShader(g_device,
        ID3D10Blob_GetBufferPointer(vs_blob), ID3D10Blob_GetBufferSize(vs_blob),
        NULL, &g_vs);
    ID3D11Device_CreatePixelShader(g_device,
        ID3D10Blob_GetBufferPointer(ps_blob), ID3D10Blob_GetBufferSize(ps_blob),
        NULL, &g_ps);
    ID3D10Blob_Release(vs_blob);
    ID3D10Blob_Release(ps_blob);

    // Fixed pipeline state — set once, valid for the lifetime of the app
    D3D11_VIEWPORT vp = { 0, 0, (float)width, (float)height, 0.0f, 1.0f };
    ID3D11DeviceContext_RSSetViewports(g_ctx, 1, &vp);
    ID3D11DeviceContext_OMSetRenderTargets(g_ctx, 1, &g_rtv, NULL);
    ID3D11DeviceContext_VSSetShader(g_ctx, g_vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_ctx, g_ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(g_ctx, 0, 1, &g_srv);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

void platform_draw_surface(Surface *s) {
    // Upload CPU pixels to the staging texture
    ID3D11DeviceContext_UpdateSubresource(g_ctx, (ID3D11Resource *)g_texture,
                                          0, NULL, s->pixels, (UINT)(s->width * 4), 0);

    // Fullscreen triangle samples the texture and writes to the back buffer
    ID3D11DeviceContext_Draw(g_ctx, 3, 0);

    IDXGISwapChain_Present(g_swap_chain, 1, 0);
}
