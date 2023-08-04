#include "os.h"
#include "common.h"
#include "memory.h"

#include <X11/X.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
} OsWindow;

struct OsWindow *os_window_create(struct Arena *arena, char *title, u32 x, u32 y, u32 w, u32 h) {
    OsWindow *window = arena_push_struct(arena, OsWindow, 8);
    
    Display *d = g_x11_display;
    
    window->width = w;
    window->height = h;
    window->os_window = XCreateSimpleWindow(d, DefaultRootWindow(d), x, y, w, h, 1, 0, 0);
    XStoreName(d, window->os_window, title);
    XSelectInput(d, window->os_window, StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask);
    XMapWindow(d, window->os_window);
    
    /* TODO: investigate why is necesary to do this to catch WM_DELETE_WINDOW message */
    window->delete_message = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, window->os_window, &window->delete_message, 1);

    XFlush(d);

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
    *x = CLAMP(window->mouse_x, 0, window->width);
    *y = CLAMP(window->mouse_y, 0, window->height);
}

void os_window_get_mouse_lbutton_state(struct OsWindow *window, b32 *is_down) {
    *is_down = window->mouse_button_down;
}

void os_window_poll_events(struct OsWindow *window) {
    
    UNUSED(window);

    Display *d = g_x11_display;
    XEvent e; 

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

