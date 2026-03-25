#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdlib.h>
#include "platform.h"

static NSWindow                  *g_window;
static CAMetalLayer              *g_layer;
static id<MTLDevice>              g_device;
static id<MTLCommandQueue>        g_queue;
static id<MTLTexture>             g_texture;
static id<MTLRenderPipelineState> g_pipeline;
static bool                       g_running = true;

// Shaders compiled at runtime — no .metal files or xcrun needed.
static NSString *kShaderSrc = @
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct V2F { float4 pos [[position]]; };\n"
    "vertex V2F vert_main(uint vid [[vertex_id]]) {\n"
    "    const float2 v[3] = {float2(-1,1), float2(3,1), float2(-1,-3)};\n"
    "    V2F out; out.pos = float4(v[vid], 0, 1); return out;\n"
    "}\n"
    "fragment float4 frag_main(V2F in [[stage_in]],\n"
    "                          texture2d<float> tex [[texture(0)]]) {\n"
    "    return tex.read(uint2(in.pos.xy));\n"
    "}\n";

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation AppDelegate
- (void)windowWillClose:(NSNotification *)notification {
    g_running = false;
}
@end

void platform_open_window(int width, int height, const char *title) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    AppDelegate *delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:delegate];

    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    NSRect frame = NSMakeRect(0, 0, width / scale, height / scale);
    NSUInteger style = NSWindowStyleMaskTitled
                     | NSWindowStyleMaskClosable
                     | NSWindowStyleMaskMiniaturizable;

    g_window = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    [g_window setTitle:[NSString stringWithUTF8String:title]];
    [g_window setDelegate:delegate];

    // Plain NSView backed by a CAMetalLayer
    NSView *view = [[NSView alloc] initWithFrame:frame];
    view.wantsLayer = YES;
    g_layer = [CAMetalLayer layer];
    view.layer = g_layer;
    [g_window setContentView:view];

    // Metal device and queue
    g_device = MTLCreateSystemDefaultDevice();
    g_queue  = [g_device newCommandQueue];

    // Layer configuration — fix drawable size to the logical pixel dimensions
    g_layer.device       = g_device;
    g_layer.pixelFormat  = MTLPixelFormatRGBA8Unorm;
    g_layer.drawableSize = CGSizeMake(width, height);

    // Staging texture: CPU-writable, GPU-readable, same format as the layer.
    // Use Shared on Apple Silicon (unified memory), Managed on Intel (discrete GPU).
    MTLTextureDescriptor *desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                          width:width
                                                         height:height
                                                      mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;
    desc.storageMode = g_device.hasUnifiedMemory
                     ? MTLStorageModeShared
                     : MTLStorageModeManaged;
    g_texture = [g_device newTextureWithDescriptor:desc];

    // Compile shaders from source at startup
    NSError *err = nil;
    id<MTLLibrary> lib = [g_device newLibraryWithSource:kShaderSrc options:nil error:&err];
    NSCAssert(lib != nil, @"Metal shader compile error: %@", err);

    MTLRenderPipelineDescriptor *rpd = [[MTLRenderPipelineDescriptor alloc] init];
    rpd.vertexFunction                    = [lib newFunctionWithName:@"vert_main"];
    rpd.fragmentFunction                  = [lib newFunctionWithName:@"frag_main"];
    rpd.colorAttachments[0].pixelFormat   = MTLPixelFormatRGBA8Unorm;
    g_pipeline = [g_device newRenderPipelineStateWithDescriptor:rpd error:&err];
    NSCAssert(g_pipeline != nil, @"Metal pipeline error: %@", err);

    [g_window center];
    [g_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

bool platform_running(void) {
    return g_running;
}

void platform_pump_events(void) {
    NSEvent *event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate distantPast]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES])) {
        [NSApp sendEvent:event];
    }
    [NSApp updateWindows];
}

void platform_draw_surface(Surface *s) {
    // 1. Upload CPU pixels to the staging texture
    [g_texture replaceRegion:MTLRegionMake2D(0, 0, s->width, s->height)
                 mipmapLevel:0
                   withBytes:s->pixels
                 bytesPerRow:s->width * 4];

    // 2. Get the next drawable (blocks briefly if the GPU is behind)
    id<CAMetalDrawable> drawable = [g_layer nextDrawable];
    if (!drawable) return;

    // 3. One-triangle render pass: sample staging texture → drawable
    MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];
    rpd.colorAttachments[0].texture     = drawable.texture;
    rpd.colorAttachments[0].loadAction  = MTLLoadActionDontCare;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLCommandBuffer>        cmd = [g_queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
    [enc setRenderPipelineState:g_pipeline];
    [enc setFragmentTexture:g_texture atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [enc endEncoding];

    [cmd presentDrawable:drawable];
    [cmd commit];
}
