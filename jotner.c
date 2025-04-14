// Includes
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#define __USE_XOPEN_EXTENDED // For usleep in c23
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

// METADATA
#define VERSION "1.0"
#define MIN_ZOOM 0.001
#define TERM_RED "\033[31m"
#define TERM_DEF "\033[0m"

// OpenGL Function Decleration
#define EXT_FUNC(type, name) static type name = 0
#include "gl_func.h"
#undef EXT_FUNC

// Helper Macros
#define ERR(x, ...) fprintf(stderr, TERM_RED "ERROR AT SOURCE CODE LINE: %u\n"\
        TERM_DEF x "\n", __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define TRY(x, ...) do {Error __err_val__; if ((__err_val__ = x)){return __err_val__;}} while(0)
#define GLTRY(x) x; do {if (glGetError() != GL_NO_ERROR) {ERR(); return GL_ERR;}} while (0)
#define THROW(x, ...) do {ERR(__VA_ARGS__); return x;} while(0)
#define max(x, y) ((x > y)? x : y)

// Types
typedef enum {
    OK = 0,
    MEM_ERR = 7,
    ARG_ERR = 8,
    SHADER_ERR,
    X_ERR,
    GL_ERR,
} Error;

typedef struct {
    Display *dpy;
    Window root;
    GLXContext ctx;
    Window wind;
    Atom close;
} App;

typedef struct {
    float x, y;
} Vec2f;

typedef struct {
    Vec2f padd;
    float zoom;
} Transform;

typedef struct {
    int32_t refresh_rate;
    char *exe_name;
    float zoom_sensitivity;
    float padd_sensitivity;
} Config;

// Data
static const char img_v_shade_src[] = {
    #embed "image.vert.glsl"
};
static const char img_f_shade_src[] = {
    #embed "image.frag.glsl"
};
static App app = {0};
static Config config;
static Transform tf = {0};
static uint32_t screen_w, screen_h;
static uint32_t mesh;

// Helper Functions
static uint8_t vec2f_eql(const Vec2f *const a, const Vec2f *const b) {
    return a->x == b->x && a->y == b->y;
}

static Error stoi(const char *text, int32_t *data) {
    *data = atoi(text);
    if (!*data) {
        return ARG_ERR;
    }

    return OK;
}

static Error stof(const char *text, float *data) {
    *data = atof(text);
    if (!*data) {
        return ARG_ERR;
    }

    return OK;
}

// Arguments
static void arg_version() {
    printf("%s-v" VERSION "\n", config.exe_name);
}

static Error arg_fps(char *fps) {
    int frame_rate;
    TRY(stoi(fps, &frame_rate));
    config.refresh_rate = 1e6 / (double)frame_rate;

    return OK;
}

static Error arg_zoom(char *sensitivity) {
    TRY(stof(sensitivity, &config.zoom_sensitivity));
    return OK;
}

static Error arg_padd(char *sensitivity) {
    TRY(stof(sensitivity, &config.padd_sensitivity));
    return OK;
}

static void arg_help() {
    printf(
            "Usage: %s [OPTIONS]\n"
            "  -h, --help                               show this help and exit\n"
            "  -v, --version                            show the version of program and exit\n"
            "  -f, --frames-per-second <int32=200>      the frame rate of the app (recommended 2x the frame rate of monitor)\n"
            "  -z, --zoom-sensitivity <float32=0.1>     scroll sensitivity for zooming\n"
            "  -p, --panning-sensitivity <float32=1.0>  mouse cursor sensitivity for panning the camera\n",
            config.exe_name
            );
}

// App Functions
static Error init_app() {
    app.dpy = XOpenDisplay(0);
    if (!app.dpy)
        THROW(X_ERR, "Cannot open display");

    int32_t screen = XDefaultScreen(app.dpy);
    screen_w = XDisplayWidth(app.dpy, screen);
    screen_h = XDisplayHeight(app.dpy, screen);
    app.root = XRootWindow(app.dpy, screen);

    int32_t vs_attrib[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    XVisualInfo *vs_info = glXChooseVisual(app.dpy, screen, vs_attrib);
    if (!vs_info) {
        XCloseDisplay(app.dpy);
        THROW(X_ERR, "Cannot retrieve visual info");
    }

    Colormap color_map = XCreateColormap(app.dpy, app.root, vs_info->visual, AllocNone);
    XSetWindowAttributes attribs;
    attribs.colormap = color_map;
    attribs.event_mask =
        ExposureMask |
        KeyPressMask | ButtonPressMask |
        KeyReleaseMask | ButtonReleaseMask |
        FocusChangeMask |
        PointerMotionMask |
        ButtonMotionMask;
    attribs.override_redirect = 1;

    app.wind = XCreateWindow(app.dpy, app.root, 0, 0, screen_w, screen_h, 0, vs_info->depth, InputOutput, vs_info->visual,
                              CWColormap | CWEventMask | CWOverrideRedirect, &attribs);
    if (!app.wind) 
        THROW(X_ERR, "Cannot create window");

    app.close = XInternAtom(app.dpy,
                                     "WM_DELETE_WINDOW",
                                     0);
    XSetWMProtocols(app.dpy, app.wind, &app.close, 1);
    XMapWindow(app.dpy, app.wind);
    XSetInputFocus(app.dpy, app.wind, RevertToPointerRoot, CurrentTime);

#define EXT_FUNC(type, name) name = (type)glXGetProcAddress((unsigned char*)#name)
    PFNGLXCREATECONTEXTATTRIBSARBPROC
        EXT_FUNC(PFNGLXCREATECONTEXTATTRIBSARBPROC,
                glXCreateContextAttribsARB);

    int32_t gl_attr[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    int fb_attr[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        None
    };

    int fbn = 0;
    GLXFBConfig *fbc = glXChooseFBConfig(app.dpy, screen, fb_attr, &fbn);
    if (!fbc)
        THROW(X_ERR, "Cannot get glx frame buffer config");

    app.ctx = glXCreateContextAttribsARB(app.dpy, fbc[0], NULL, GL_TRUE, gl_attr);
    if (!glXMakeCurrent(app.dpy, app.wind, app.ctx))
        THROW(X_ERR, "Cannot use OpenGL context");
 
#include "gl_func.h"
#undef EXT_FUNC

    return OK;
}

static void init_config() {
    config = (Config) {
        .refresh_rate = 1e6/240,
        .zoom_sensitivity = 0.2,
        .padd_sensitivity = 1,
    };
} 

static Error terminate() {
    if (app.ctx) {
        GLTRY(glBindVertexArray(0));
        GLTRY(glDeleteVertexArrays(1, &mesh));
        glXMakeCurrent(app.dpy, None, NULL);
        glXDestroyContext(app.dpy, app.ctx);
    }
    if (app.wind)
        XDestroyWindow(app.dpy, app.wind);
    if (app.dpy)
        XCloseDisplay(app.dpy);
    return OK;
}

static Error screenshot() {
    XImage *img = XGetImage(app.dpy, app.wind, 0, 0, screen_w, screen_h, AllPlanes, ZPixmap);
    if (!img)
        return X_ERR;

    GLuint texture;
    GLTRY(glGenTextures(1, &texture));
    GLTRY(glBindTexture(GL_TEXTURE_2D, texture));

    GLubyte *img_data = malloc(screen_w * screen_w * 4);
    if (!img_data)
        return MEM_ERR;

    for (uint32_t x = 0; x < screen_w; x++) {
        for (uint32_t y = 0; y < screen_h; y++) {
            uint32_t pixel = XGetPixel(img, x, screen_h - y - 1);
            img_data[(x + y * screen_w) * 4 + 0] =
                            (pixel & img->red_mask) >> 16;
            img_data[(x + y * screen_w) * 4 + 1] =
                            (pixel & img->green_mask) >> 8;
            img_data[(x + y * screen_w) * 4 + 2] =
                            (pixel & img->blue_mask);
            img_data[(x + y * screen_w) * 4 + 3] = 255;
        }
    }

    GLTRY(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                screen_w, screen_h, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, img_data));
    GLTRY(glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLTRY(glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    XDestroyImage(img);
    free(img_data);

    GLTRY(glBindTexture(GL_TEXTURE_2D, texture));
    return OK;
}

static Error compile_shader(uint32_t *shade, GLenum shade_type, const char *const shade_src, const char *const shade_name) {
    *shade = glCreateShader(shade_type);
    GLTRY(glShaderSource(*shade, 1, &shade_src, 0));
    GLTRY(glCompileShader(*shade));

    int  success;
    char info_log[512];
    GLTRY(glGetShaderiv(*shade, GL_COMPILE_STATUS, &success));
    if(!success)
    {
        GLTRY(glGetShaderInfoLog(*shade, 512, NULL, info_log));
        ERR("%s SHADER: %s\n", shade_name, info_log);
        return SHADER_ERR;
    }

    return OK;
}

static Error init_mesh() {
    float verts[] = {
        -1, -1,
         1, -1,
         1,  1,
         1,  1,
        -1,  1,
        -1, -1
    };

    uint32_t vbo;
    GLTRY(glGenBuffers(1, &vbo));
    GLTRY(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLTRY(glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW));

    GLTRY(glGenVertexArrays(1, &mesh));
    GLTRY(glBindVertexArray(mesh));

    GLTRY(glEnableVertexAttribArray(0));
    GLTRY(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));
    GLTRY(glBindVertexArray(0));
    GLTRY(glDeleteBuffers(1, &vbo));

    uint32_t vert_shade;
    uint32_t frag_shade;
    TRY(compile_shader(&vert_shade, GL_VERTEX_SHADER, &img_v_shade_src[0], "VERTEX"));
    TRY(compile_shader(&frag_shade, GL_FRAGMENT_SHADER, &img_f_shade_src[0], "FRAGMENT"));

    uint32_t prog = glCreateProgram();
    GLTRY(glAttachShader(prog, vert_shade));
    GLTRY(glAttachShader(prog, frag_shade));
    GLTRY(glLinkProgram(prog));
    int32_t success;
    char info_log[512];
    GLTRY(glGetProgramiv(prog, GL_LINK_STATUS, &success));
    if(!success) {
        GLTRY(glGetProgramInfoLog(prog, 512, NULL, info_log));
        ERR("%s\n", info_log);
    }

    GLTRY(glBindVertexArray(mesh));
    GLTRY(glUseProgram(prog));
    return OK;
}

static void query_mouse(Vec2f *pos, uint32_t *button_mask) {
    Window root_ret;
    int32_t pointer_x, pointer_y;
    int32_t pointer_root_x, pointer_root_y;
    XQueryPointer(app.dpy, app.wind, &app.root,
            &root_ret, &pointer_root_x, &pointer_root_y,
            &pointer_x, &pointer_y, button_mask);

    pos->x = pointer_x  * 2.0 / screen_w - 1;
    pos->y = 1 - pointer_y * 2.0 / screen_h;
}

static void update_movement(XEvent event) {
    Vec2f mouse_pos;
    uint32_t buttons;
    static Vec2f o_padd_pos = {0};
    query_mouse(&mouse_pos, &buttons);
    if (buttons & Button1MotionMask) {
        if (!vec2f_eql(&mouse_pos, &o_padd_pos)) {
            Vec2f d_mouse;
            d_mouse.x = (mouse_pos.x - o_padd_pos.x) * config.padd_sensitivity;
            d_mouse.y = (mouse_pos.y - o_padd_pos.y) * config.padd_sensitivity;

            tf.padd.x += d_mouse.x;
            tf.padd.y += d_mouse.y;
            o_padd_pos = mouse_pos;
        }
    }
    else 
        o_padd_pos = mouse_pos;

    if (event.type == ButtonPress) {
        if (event.xbutton.button == Button4 || event.xbutton.button == Button5) {
            // Thank you DeepSeek, although i had to make some changes
            float x0 = (mouse_pos.x - tf.padd.x) / tf.zoom;
            float y0 = (mouse_pos.y - tf.padd.y) / tf.zoom;
            tf.zoom = max(tf.zoom - (config.zoom_sensitivity*tf.zoom) * (event.xbutton.button - 4.5), MIN_ZOOM);
            tf.padd.x = mouse_pos.x - x0 * tf.zoom;
            tf.padd.y = mouse_pos.y - y0 * tf.zoom;
        }
    }
}

static void update(uint8_t *should_close) {
    XEvent event;
    XNextEvent(app.dpy, &event);

    if ((Atom)event.xclient.data.l[0] == app.close ||
            (event.type == KeyPress &&
             event.xkey.keycode == XKeysymToKeycode(app.dpy, XK_Escape))) {
        *should_close = 1;
    }

    update_movement(event);
}

static Error draw() {
    GLTRY(glClear(GL_COLOR_BUFFER_BIT));
    GLTRY(glUniform1f(1, tf.zoom));
    GLTRY(glUniform2f(2, tf.padd.x, tf.padd.y));
    GLTRY(glDrawArrays(GL_TRIANGLES, 0, 6));

    glXSwapBuffers(app.dpy, app.wind);
    return OK;
}

int main(int32_t argc, char *argv[]) {
    init_config();
    config.exe_name = argv[0];
    if (argc > 1) {
        for (int32_t i = 1; i < argc; i++) {
            if (!strcmp("--version", argv[i]) ||
                    !strcmp("-v", argv[i])) {
                arg_version();
                return 0;
            }
            else if (!strcmp("--frames-per-second", argv[i]) ||
                    !strcmp("-f", argv[i])) {
                if (argc == i + 1) {
                    THROW(ARG_ERR, "Expected an integer value for fps");
                }
                TRY(arg_fps(argv[++i]));
            }
            else if (!strcmp("--zoom-sensitivity", argv[i]) ||
                    !strcmp("-z", argv[i])) {
                if (argc == i + 1) {
                    THROW(ARG_ERR, "Expected a floating point or integer value for zoom sensitivity");
                }
                TRY(arg_zoom(argv[++i]));
            }
            else if (!strcmp("--panning-sensitivity", argv[i]) ||
                    !strcmp("-p", argv[i])) {
                if (argc == i + 1) {
                    THROW(ARG_ERR, "Expected a floating point or integer value for padding sensitivity");
                }
                TRY(arg_padd(argv[++i]));
            }
            else if (!strcmp("--help", argv[i]) ||
                    !strcmp("-h", argv[i])) {
                arg_help();
                return 0;
            }
            else {
                ERR("Invalid argument \"%s\"a\n", argv[i]);
                arg_help();
                return ARG_ERR;
            }
        }
    }

    TRY(init_app());
    TRY(screenshot());
    TRY(init_mesh());
    tf.zoom = 1;

    uint8_t should_close = 0;
    while (!should_close) {
        update(&should_close);
        TRY(draw());
        usleep(config.refresh_rate);
    }

    TRY(terminate());
    return OK;
}
