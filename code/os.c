#include "os.h"
#include "common.h"
#include "memory.h"
#include "tgui.h"

#include <X11/X.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>

#include <stdlib.h>
#include "stb_truetype.h"

static u64 g_os_page_size = 0;
static Display *g_x11_display = 0;
static int g_x11_screen = 0;

static inline void os_error(void) {
    printf("OS Error: %s\n", strerror(errno));
    ASSERT(!"Fatal error!");
}

void os_initialize(void) {

    long result = sysconf(_SC_PAGESIZE); 
    if(result == -1) {
        os_error();
    }
    g_os_page_size = (u64)result; 

    g_x11_display = XOpenDisplay(NULL);
    g_x11_screen = DefaultScreen(g_x11_display);
}

void os_terminate(void) {
    XCloseDisplay(g_x11_display);
}

/* -------------------------
       File Manager 
   ------------------------- */

typedef struct OsFile {
    void *data;
    u64 size;
} OsFile;

OsFile *os_file_read_entire(const char *path) {
    OsFile *result = malloc(sizeof(OsFile));

    FILE *file = fopen((char *)path, "rb");
    if(!file) {
        printf("Cannot load file: %s\n", path);
        os_error();
    }
    
    fseek(file, 0, SEEK_END);
    u64 file_size = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    result = (OsFile *)malloc(sizeof(OsFile) + file_size);
    
    result->data = result + 1;
    result->size = file_size;

    ASSERT(((u64)result->data % 8) == 0);
    fread(result->data, file_size, 1, file);
    
    fclose(file);

    return result;
}

void os_file_free(OsFile *file) {
    free(file);
}

/* -------------------------
        Font Rasterizer 
   ------------------------- */

typedef struct OsFont {
    OsFile *file;
    stbtt_fontinfo info;
    u32 size;
    f32 size_ratio;
} OsFont;

struct OsFont *os_font_create(struct Arena *arena, const char *path, u32 size) {
    OsFont *font = arena_push_struct(arena, OsFont, 8);
    font->size = size;
    font->file = os_file_read_entire(path);
    stbtt_InitFont(&font->info, font->file->data, stbtt_GetFontOffsetForIndex(font->file->data,0));
    font->size_ratio = stbtt_ScaleForPixelHeight(&font->info, size);
    return font;
}

void os_font_destroy(struct OsFont *font) {
    os_file_free(font->file);
}

void os_font_rasterize_glyph(struct OsFont *font, u32 codepoint, void **buffer, s32 *w, s32 *h, s32 *bpp) {
   *buffer = stbtt_GetCodepointBitmap(&font->info, 0, font->size_ratio, codepoint, w, h, 0,0);
   *bpp = 1;
}

s32 os_font_get_kerning_between(struct OsFont *font, u32 codepoint0, u32 codepoint1) {
    return stbtt_GetCodepointKernAdvance(&font->info, codepoint0, codepoint1) * font->size_ratio;
}

void os_font_get_vmetrics(struct OsFont *font, s32 *ascent, s32 *descent, s32 *line_gap) { 
    stbtt_GetFontVMetrics(&font->info, ascent, descent, line_gap);
    *ascent   *= font->size_ratio;
    *descent  *= font->size_ratio;
    *line_gap *= font->size_ratio;
}

void os_font_get_glyph_metrics(struct OsFont *font, u32 codepoint, s32 *adv_width, s32 *left_bearing, s32 *top_bearing) {
    stbtt_GetCodepointHMetrics(&font->info, codepoint, adv_width, left_bearing);
    *adv_width    *= font->size_ratio;
    *left_bearing *= font->size_ratio;
    s32 x0, y0, x1, y1;
    stbtt_GetCodepointBox(&font->info, codepoint, &x0, &y0, &x1, &y1);
    y1 *= font->size_ratio;
    *top_bearing = y1;
}

/* -------------
       Time 
   ------------- */

u64 os_get_ticks(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    u64 result = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    return result;
}

void os_sleep(u64 milliseconds) {
    
    u64 seconds = milliseconds / 1000; 
    u64 nanoseconds = (milliseconds - (seconds * 1000)) * 1000000;

    struct timespec req;
    req.tv_sec = seconds; 
    req.tv_nsec = nanoseconds;
    while(nanosleep(&req, &req) == -1);
}

/* ---------------------------
        Windows and Inputs 
   --------------------------- */

#define OS_WINDOW_MAX_TEXT_SIZE 32

typedef struct OsWindow {
    s32 width;
    s32 height;
    b32 resize;
    b32 should_close;

    s32 mouse_x;
    s32 mouse_y;
    
    b32 mouse_button_down;

    Window os_window;
    Atom delete_message;

    char text_buffer[OS_WINDOW_MAX_TEXT_SIZE];
    s32 text_size;
    
    TGuiKeyboard keyboard;
    
} OsWindow;


static Cursor cursor_v_double_arrow;
static Cursor cursor_h_double_arrow;
static Cursor cursor_hand;
static Cursor cursor_arrow;
 
void os_set_cursor(OsWindow *window, OsCursor cursor) {

    Display *d = g_x11_display;
    
    switch (cursor) { 
    
    case OS_CURSOR_ARROW:   { XDefineCursor(d, window->os_window, cursor_arrow); } break;
    case OS_CURSOR_HAND:    { XDefineCursor(d, window->os_window, cursor_hand); } break;
    case OS_CURSOR_V_ARROW: { XDefineCursor(d, window->os_window, cursor_v_double_arrow); } break;
    case OS_CURSOR_H_ARROW: { XDefineCursor(d, window->os_window, cursor_h_double_arrow); } break;

    }
}

struct OsWindow *os_window_create(struct Arena *arena, char *title, u32 x, u32 y, u32 w, u32 h) {
    OsWindow *window = arena_push_struct(arena, OsWindow, 8);
    memset(window, 0, sizeof(OsWindow));
    
    Display *d = g_x11_display;
    
    window->width = w;
    window->height = h;
    window->os_window = XCreateSimpleWindow(d, DefaultRootWindow(d), x, y, w, h, 1, 0, 0);
    XStoreName(d, window->os_window, title);
    XSelectInput(d, window->os_window, StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|KeymapStateMask);
    XMapWindow(d, window->os_window);
    
    /* TODO: investigate why is necesary to do this to catch WM_DELETE_WINDOW message */
    window->delete_message = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, window->os_window, &window->delete_message, 1);
    
    XFlush(d);

    cursor_v_double_arrow = XcursorLibraryLoadCursor(d, "sb_v_double_arrow");
    cursor_h_double_arrow = XcursorLibraryLoadCursor(d, "sb_h_double_arrow");
    cursor_hand           = XcursorLibraryLoadCursor(d, "hand1");
    cursor_arrow          = XcursorLibraryLoadCursor(d, "arrow");
    XDefineCursor (d, window->os_window, cursor_arrow);

    return window;
}

void os_window_destroy(struct OsWindow *window) {
    XDestroyWindow(g_x11_display, window->os_window);
}

s32 os_window_width(struct OsWindow *window) {
    return window->width;
}

s32 os_window_height(struct OsWindow *window) {
    return window->height;
}

b32 os_window_should_close(struct OsWindow *window) {
    return window->should_close;
}

b32 os_window_resize(struct OsWindow *window) {
    return window->resize;
}

void os_window_get_mouse_position(struct OsWindow *window, s32 *x, s32 *y) {
    /* TODO: Handle mouse clamp in the docker code */
    *x = CLAMP(window->mouse_x, 0, window->width   - 1);
    *y = CLAMP(window->mouse_y, 0, window->height - 1);
}

void os_window_get_mouse_lbutton_state(struct OsWindow *window, b32 *is_down) {
    *is_down = window->mouse_button_down;
}

void os_window_get_text_input(struct OsWindow *window, u8 *buffer, u32 *size, u32 max_size) {
    if(window->text_size > 0) {
        *size = MIN((u32)window->text_size, max_size);
        memcpy(buffer, window->text_buffer, *size);
    } else {
        *size = 0;
    }
}


void os_window_get_keyboard_input(struct OsWindow *window, struct TGuiKeyboard *keyborad) {
    *keyborad = window->keyboard;
}

void os_window_poll_events(struct OsWindow *window) {
    
    Display *d = g_x11_display;
    XEvent e; 
    TGuiKeyboard *keyboard = &window->keyboard;
    window->text_size = 0;
    memset(keyboard, 0, sizeof(TGuiKeyboard));

    while(XPending(d)) {
     
        XNextEvent(d, &e);
        
        switch(e.type) {
        
        case ClientMessage: {
            if (e.xclient.data.l[0] == (long int)window->delete_message) {
                window->should_close = true;
            }
        } break;
       
        case ConfigureNotify: {
            XConfigureEvent xce = e.xconfigure;
            if (xce.width != window->width || xce.height != window->height) {
                window->resize = true;
                window->width = xce.width;
                window->height = xce.height;
            }
        } break;

        case Expose: {

        } break;
        
        case KeymapNotify: {
            XRefreshKeyboardMapping(&e.xmapping);
        } break;
        
        case KeyPress: {

            KeySym sym = XLookupKeysym(&e.xkey, 0);
            
            keyboard->k_shift = (e.xkey.state & ShiftMask)   != 0;
            keyboard->k_ctrl  = (e.xkey.state & ControlMask) != 0;

            if(sym == XK_Right) {
            
                keyboard->k_r_arrow_down = true;
            
            } else if(sym == XK_Left) {
            
                keyboard->k_l_arrow_down = true;
            
            } else if(sym == XK_BackSpace) {
            
                keyboard->k_backspace = true;
            
            } else if(sym == XK_Delete) {
            
                keyboard->k_delete = true;
            
            } else {
                window->text_size = XLookupString(&e.xkey, window->text_buffer, OS_WINDOW_MAX_TEXT_SIZE, &sym, NULL);
                if(window->text_size < 0) window->text_size = 0;
            }

        } break;

        case ButtonPress: {
            if(e.xbutton.button == 1) {
                window->mouse_button_down = true;
            }
        } break;
        case ButtonRelease: {
            if(e.xbutton.button == 1) {
                window->mouse_button_down = false;
            }
        } break;
        
        
        }
    }

    Window inwin;
    Window inchildwin;
    int rootx, rooty;
    int childx, childy;
    unsigned int mask;
    XQueryPointer(d, window->os_window, &inwin, &inchildwin, &rootx, &rooty, &childx, &childy, &mask);
    window->mouse_x = childx;
    window->mouse_y = childy;
    
}

/* ---------------------
        Backbuffer 
   --------------------- */

typedef struct OsBackbuffer {
    u8 *data;
    u32 width;
    u32 height;
    u32 bytes_per_pixel;
    GC gc;
    XImage *image;
} OsBackbuffer;

struct OsBackbuffer *os_backbuffer_create(struct Arena *arena, struct OsWindow *window, void *data, u32 w, u32 h, u32 bytes_per_pixel) {
    
    ASSERT(bytes_per_pixel == 1 || bytes_per_pixel == 2 || bytes_per_pixel == 4);

    OsBackbuffer *backbuffer = arena_alloc(arena, sizeof(OsBackbuffer), 8);

    Display *d = g_x11_display;
    int s = g_x11_screen;

    Visual *visual = DefaultVisual(d, s);

    backbuffer->width  = w;
    backbuffer->height = h;
    backbuffer->data = data;    
    backbuffer->bytes_per_pixel = bytes_per_pixel;

    backbuffer->image = XCreateImage(d, visual, DefaultDepth(d, s), ZPixmap, 0, (char *)backbuffer->data, w, h, bytes_per_pixel*8, 0);
    
    ASSERT(backbuffer->image);
    
    backbuffer->gc = XCreateGC(d, window->os_window, 0, NULL);

    XFlush(d);

    return backbuffer;
}

void os_backbuffer_destroy(struct OsBackbuffer *backbuffer) {
    Display *d = g_x11_display;
    backbuffer->image->data = 0;
    XDestroyImage(backbuffer->image);
    XFreeGC(d, backbuffer->gc);
}

void os_backbuffer_swap(struct OsWindow *window, struct OsBackbuffer *backbuffer) {
    Display *d = g_x11_display;
    XPutImage(d, window->os_window, backbuffer->gc, backbuffer->image, 0, 0, 0, 0, backbuffer->width, backbuffer->height);
    XFlush(d);
}

/* -------------------
        Memory
   ------------------- */

u64 os_get_page_size(void) {
    ASSERT(g_os_page_size > 0);
    return g_os_page_size;
}

void *os_virtual_reserve(u64 size) {

    ASSERT(IS_POWER_OF_TWO(os_get_page_size()));
    ASSERT((size & (os_get_page_size() - 1)) == 0);

    int prot = PROT_NONE;
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    off_t offset = 0;
    void *address = mmap(NULL, (size_t)size, prot, flags, -1, offset);
    
    if(address == MAP_FAILED) {
        os_error();
    }

    return address;
}

void os_virtual_commit(void *ptr, u64 size) {

    ASSERT(IS_POWER_OF_TWO(os_get_page_size()));
    ASSERT(((u64)ptr & (os_get_page_size() - 1)) == 0);
    ASSERT((size & (os_get_page_size() - 1)) == 0);

    int prot = PROT_READ | PROT_WRITE;
    int result = 0;
    
    result = madvise(ptr, size, MADV_WILLNEED);
    if(result == -1) {
        os_error();
    }
    
    result = mprotect(ptr, (size_t)size, prot);
    if(result == -1) {
        os_error();
    }
}

void os_virtual_decommit(void *ptr, u64 size) {
    
    ASSERT(IS_POWER_OF_TWO(os_get_page_size()));
    ASSERT(((u64)ptr & (os_get_page_size() - 1)) == 0);
    ASSERT((size & (os_get_page_size() - 1)) == 0);

    int prot = PROT_NONE;
    int result = 0;
    
    result = madvise(ptr, size, MADV_DONTNEED);
    if(result == -1) {
        os_error();
    }

    result = mprotect(ptr, (size_t)size, prot);
    if(result == -1) {
        os_error();
    }
}

void os_virtual_release(void *ptr, u64 size) {
    
    ASSERT(IS_POWER_OF_TWO(os_get_page_size()));
    ASSERT(((u64)ptr & (os_get_page_size() - 1)) == 0);
    ASSERT((size & (os_get_page_size() - 1)) == 0);
    
    int result = munmap(ptr, size);
    if(result == -1) {
        os_error();
    }
}

