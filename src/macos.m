#import <Cocoa/Cocoa.h>
#include "platform.h"

@interface GameView : NSView
@end

@implementation GameView
- (void)drawRect:(NSRect)dirtyRect {
    Color c = PLATFORM_CLEAR_COLOR;
    [[NSColor colorWithRed:c.r/255.0 green:c.g/255.0 blue:c.b/255.0 alpha:1.0] set];
    NSRectFill(dirtyRect);
}
@end

static NSWindow *g_window = nil;
static bool g_running = true;

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

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSUInteger style = NSWindowStyleMaskTitled
                     | NSWindowStyleMaskClosable
                     | NSWindowStyleMaskMiniaturizable;

    g_window = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];

    [g_window setTitle:[NSString stringWithUTF8String:title]];
    [g_window setDelegate:delegate];

    GameView *view = [[GameView alloc] initWithFrame:frame];
    [g_window setContentView:view];

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
