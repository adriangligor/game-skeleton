#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glext.h>
#include <stdlib.h>
#include <assert.h>
#include "platform.h"

// GLX_ARB_create_context constants (defined in glxext.h, guard against
// systems where glx.h already pulls them in)
#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#define GLX_CONTEXT_MAJOR_VERSION_ARB    0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB    0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB     0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

typedef GLXContext (*glXCreateContextAttribsARBProc)(
    Display *, GLXFBConfig, GLXContext, Bool, const int *);

static Display *g_display;
static Window g_window;
static Atom g_wm_delete_window;
static GLXContext g_ctx;
static GLuint g_texture;
static GLuint g_program;
static GLuint g_vao;
static bool g_running;

// Fullscreen triangle. OpenGL UV origin is bottom-left but our CPU buffer
// has row 0 at the top, so V is flipped: uv.y = (1 - ndc.y) / 2.
static const char *k_vert =
    "#version 330 core\n"
    "out vec2 uv;\n"
    "void main() {\n"
    "    vec2 v[3] = vec2[](vec2(-1,1), vec2(3,1), vec2(-1,-3));\n"
    "    uv = vec2((v[gl_VertexID].x + 1.0) * 0.5,\n"
    "              (1.0 - v[gl_VertexID].y) * 0.5);\n"
    "    gl_Position = vec4(v[gl_VertexID], 0.0, 1.0);\n"
    "}\n";

static const char *k_frag =
    "#version 330 core\n"
    "in vec2 uv;\n"
    "uniform sampler2D tex;\n"
    "out vec4 color;\n"
    "void main() { color = texture(tex, uv); }\n";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

void platform_open_window(int width, int height, const char *title) {
    g_display = XOpenDisplay(NULL);
    int screen = DefaultScreen(g_display);

    // Pick a framebuffer config: RGBA, double-buffered
    int fb_attribs[] = {
        GLX_X_RENDERABLE,  True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER,  True,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
        None
    };
    int n;
    GLXFBConfig *fbcs = glXChooseFBConfig(g_display, screen, fb_attribs, &n);
    assert(n > 0);
    GLXFBConfig fbc = fbcs[0];
    XFree(fbcs);

    // Window must use the visual that matches the FB config
    XVisualInfo *vi = glXGetVisualFromFBConfig(g_display, fbc);
    Colormap cmap = XCreateColormap(g_display, RootWindow(g_display, screen),
                                    vi->visual, AllocNone);
    XSetWindowAttributes swa = {
        .colormap   = cmap,
        .event_mask = StructureNotifyMask,
    };
    g_window = XCreateWindow(g_display, RootWindow(g_display, screen),
                             0, 0, width, height, 0,
                             vi->depth, InputOutput, vi->visual,
                             CWColormap | CWEventMask, &swa);
    XFree(vi);

    XStoreName(g_display, g_window, title);

    XSizeHints *hints = XAllocSizeHints();
    hints->flags      = PMinSize | PMaxSize;
    hints->min_width  = hints->max_width  = width;
    hints->min_height = hints->max_height = height;
    XSetWMNormalHints(g_display, g_window, hints);
    XFree(hints);

    g_wm_delete_window = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_wm_delete_window, 1);

    XMapWindow(g_display, g_window);
    XFlush(g_display);

    // OpenGL 3.3 core context (requires GLX_ARB_create_context extension)
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBProc)
        glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    int ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };
    g_ctx = glXCreateContextAttribsARB(g_display, fbc, NULL, True, ctx_attribs);
    glXMakeCurrent(g_display, g_window, g_ctx);

    // Compile and link the fullscreen blit shader
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   k_vert);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, k_frag);
    g_program = glCreateProgram();
    glAttachShader(g_program, vert);
    glAttachShader(g_program, frag);
    glLinkProgram(g_program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glUseProgram(g_program);
    glUniform1i(glGetUniformLocation(g_program, "tex"), 0);

    // Empty VAO — required by OpenGL 3.3 core for any draw call
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);

    // RGBA8 texture — same format as the CPU surface
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glViewport(0, 0, width, height);

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

void platform_draw_surface(Surface *s) {
    // Upload CPU pixels to the GL texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    s->width, s->height,
                    GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);

    // Fullscreen triangle samples the texture and writes to the framebuffer
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glXSwapBuffers(g_display, g_window);
}
