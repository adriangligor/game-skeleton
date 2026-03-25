#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glext.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <assert.h>
#include "platform.h"
#include "input.h"

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

static KeyCode to_keycode(KeySym sym) {
    switch (sym) {
        case XK_a:
        case XK_A:
            return KEY_A;
        case XK_b:
        case XK_B:
            return KEY_B;
        case XK_c:
        case XK_C:
            return KEY_C;
        case XK_d:
        case XK_D:
            return KEY_D;
        case XK_e:
        case XK_E:
            return KEY_E;
        case XK_f:
        case XK_F:
            return KEY_F;
        case XK_g:
        case XK_G:
            return KEY_G;
        case XK_h:
        case XK_H:
            return KEY_H;
        case XK_i:
        case XK_I:
            return KEY_I;
        case XK_j:
        case XK_J:
            return KEY_J;
        case XK_k:
        case XK_K:
            return KEY_K;
        case XK_l:
        case XK_L:
            return KEY_L;
        case XK_m:
        case XK_M:
            return KEY_M;
        case XK_n:
        case XK_N:
            return KEY_N;
        case XK_o:
        case XK_O:
            return KEY_O;
        case XK_p:
        case XK_P:
            return KEY_P;
        case XK_q:
        case XK_Q:
            return KEY_Q;
        case XK_r:
        case XK_R:
            return KEY_R;
        case XK_s:
        case XK_S:
            return KEY_S;
        case XK_t:
        case XK_T:
            return KEY_T;
        case XK_u:
        case XK_U:
            return KEY_U;
        case XK_v:
        case XK_V:
            return KEY_V;
        case XK_w:
        case XK_W:
            return KEY_W;
        case XK_x:
        case XK_X:
            return KEY_X;
        case XK_y:
        case XK_Y:
            return KEY_Y;
        case XK_z:
        case XK_Z:
            return KEY_Z;
        case XK_0:
            return KEY_0;
        case XK_1:
            return KEY_1;
        case XK_2:
            return KEY_2;
        case XK_3:
            return KEY_3;
        case XK_4:
            return KEY_4;
        case XK_5:
            return KEY_5;
        case XK_6:
            return KEY_6;
        case XK_7:
            return KEY_7;
        case XK_8:
            return KEY_8;
        case XK_9:
            return KEY_9;
        case XK_space:
            return KEY_SPACE;
        case XK_Return:
            return KEY_ENTER;
        case XK_Escape:
            return KEY_ESCAPE;
        case XK_Tab:
            return KEY_TAB;
        case XK_Up:
            return KEY_UP;
        case XK_Down:
            return KEY_DOWN;
        case XK_Left:
            return KEY_LEFT;
        case XK_Right:
            return KEY_RIGHT;
        default:
            return KEY_UNKNOWN;
    }
}

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
        .colormap = cmap,
        .event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask,
    };
    g_window = XCreateWindow(g_display, RootWindow(g_display, screen),
        0, 0, width, height, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa);
    XFree(vi);

    XStoreName(g_display, g_window, title);

    XSizeHints *hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = hints->max_width = width;
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
            input_push_event((Event){ .type = EVENT_SYSTEM_QUIT });
        }
        else if (e.type == KeyPress) {
            KeySym sym = XLookupKeysym(&e.xkey, 0);
            input_push_event((Event){ .type = EVENT_KEY_DOWN, .key = to_keycode(sym) });
        }
        else if (e.type == KeyRelease) {
            KeySym sym = XLookupKeysym(&e.xkey, 0);
            input_push_event((Event){ .type = EVENT_KEY_UP, .key = to_keycode(sym) });
        }
    }
}

void platform_quit(void) {
    g_running = false;
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
