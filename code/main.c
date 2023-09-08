#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>

#include <time.h>
#include <sys/time.h>

#include "os_gl.h"

#include "tgui.h"

#define ASSERT(expr) assert((expr))
#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))

#define true  1
#define false 0


static Display *g_x11_display = 0;
static int g_x11_screen = 0;

typedef struct OsFile {
    void *data;
    tgui_u64 size;
} OsFile;

OsFile *os_file_read_entire(const char *path) {
    OsFile *result = malloc(sizeof(OsFile));

    FILE *file = fopen((char *)path, "rb");
    if(!file) {
        printf("Cannot load file: %s\n", path);
        free(result);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    tgui_u64 file_size = (tgui_u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    result = (OsFile *)malloc(sizeof(OsFile) + file_size+1);
    
    result->data = result + 1;
    result->size = file_size;

    ASSERT(((tgui_u64)result->data % 8) == 0);
    fread(result->data, file_size+1, 1, file);
    ((tgui_u8 *)result->data)[file_size] = '\0';
    
    fclose(file);

    return result;
}

void os_file_free(OsFile *file) {
    free(file);
}

tgui_u64 os_get_ticks(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tgui_u64 result = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    return result;
}

void os_sleep(tgui_u64 milliseconds) {
    
    tgui_u64 seconds = milliseconds / 1000; 
    tgui_u64 nanoseconds = (milliseconds - (seconds * 1000)) * 1000000;

    struct timespec req;
    req.tv_sec = seconds; 
    req.tv_nsec = nanoseconds;
    while(nanosleep(&req, &req) == -1);
}

typedef struct OsWindow {
    tgui_s32 width;
    tgui_s32 height;
    tgui_b32 resize;
    tgui_b32 should_close;

    Window os_window;
    Atom delete_message;

    Colormap cm;
    GLXContext ctx;
    GLXFBConfig best_fbc;
    
} OsWindow;

static Cursor cursor_v_double_arrow;
static Cursor cursor_h_double_arrow;
static Cursor cursor_hand;
static Cursor cursor_arrow;

void os_initialize(void) {

    g_x11_display = XOpenDisplay(NULL);
    g_x11_screen = DefaultScreen(g_x11_display);
}

void os_terminate(void) {
    XCloseDisplay(g_x11_display);
}

struct OsWindow *os_window_create(char *title, tgui_u32 x, tgui_u32 y, tgui_u32 w, tgui_u32 h) {

    OsWindow *window = (OsWindow *)malloc(sizeof(OsWindow));
    memset(window, 0, sizeof(OsWindow));
    
    window->resize = true;
    window->width = w;
    window->height = h;

    Display *d = g_x11_display;
  
    static int fb_attr[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
  
    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(d, DefaultScreen(d), fb_attr, &fbcount);
    if (!fbc) {
        printf("Failed to retrieve a X11 framebuffer config\n");
        return NULL;
    }
  
    /* Pick the FB config/visual with the most samples per pixel */
    int best_fbc_index = -1, worst_fbc_index = -1, best_num_samp = -1, worst_num_samp = 999;
  
    for(int i = 0; i < fbcount; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(d, fbc[i]);
        if(vi) {
            int samp_buf, samples;
            glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(d, fbc[i], GLX_SAMPLES, &samples);
            
           if((best_fbc_index < 0 || samp_buf) && samples > best_num_samp)
               best_fbc_index = i, best_num_samp = samples;
           if((worst_fbc_index < 0 || !samp_buf) || samples < worst_num_samp)
               worst_fbc_index = i, worst_num_samp = samples;

        }
        XFree(vi);
    }

    window->best_fbc = fbc[best_fbc_index];
    XFree(fbc);
    XVisualInfo *vi = glXGetVisualFromFBConfig(d, window->best_fbc);
  
    XSetWindowAttributes swa;
    window->cm = XCreateColormap(d, RootWindow(d, vi->screen), vi->visual, AllocNone);

    long event_mask = StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|KeymapStateMask;
    swa.colormap = window->cm;
    swa.background_pixmap = None ;
    swa.border_pixel      = 0;
    swa.event_mask        = event_mask;

    window->os_window = XCreateWindow(d, RootWindow(d, vi->screen), 
                            x, y, window->width, window->height, 0, vi->depth, InputOutput, vi->visual, 
                            CWBorderPixel|CWColormap|CWEventMask, &swa);

    XStoreName(d, window->os_window, title);
    XSelectInput(d, window->os_window, event_mask);
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
    Display *d = g_x11_display;
    XFreeColormap(d, window->cm);
    XDestroyWindow(d, window->os_window);
    free(window);
}

void os_window_poll_events(struct OsWindow *window, TGuiInput *input) {
    
    Display *d = g_x11_display;
    XEvent e; 

    memset(&input->keyboard, 0, sizeof(input->keyboard));
    input->text_size = 0;

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
                
                input->window_resize = true;
                input->resize_w = window->width;
                input->resize_h = window->height;

            }
        } break;

        case Expose: {

        } break;
        
        case KeymapNotify: {
            XRefreshKeyboardMapping(&e.xmapping);
        } break;
        
        case KeyPress: {

            KeySym sym = XLookupKeysym(&e.xkey, 0);
            
            input->keyboard.k_shift = (e.xkey.state & ShiftMask)   != 0;
            input->keyboard.k_ctrl  = (e.xkey.state & ControlMask) != 0;
            

            if(sym == XK_Right) {
                input->keyboard.k_r_arrow_down = true;
            } else if(sym == XK_Left) {
                input->keyboard.k_l_arrow_down = true;
            } else if(sym == XK_BackSpace) {
                input->keyboard.k_backspace = true;
            } else if(sym == XK_Delete) {
                input->keyboard.k_delete= true;
            } else {

                input->text_size = XLookupString(&e.xkey, (char *)input->text, TGUI_MAX_TEXT_SIZE, &sym, NULL);
                if(input->text_size < 0) input->text_size = 0;
            }

        } break;

        case ButtonPress: {
            if(e.xbutton.button == 1) {
                input->mouse_button_is_down = true;
            }
        } break;
        case ButtonRelease: {
            if(e.xbutton.button == 1) {
                input->mouse_button_is_down = false;
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
    input->mouse_x = childx;
    input->mouse_y = childy;

    switch (tgui_get_cursor_state()) { 
    case TGUI_CURSOR_ARROW:   { XDefineCursor(d, window->os_window, cursor_arrow); } break;         
    case TGUI_CURSOR_HAND:    { XDefineCursor(d, window->os_window, cursor_hand); } break;          
    case TGUI_CURSOR_V_ARROW: { XDefineCursor(d, window->os_window, cursor_v_double_arrow); } break;
    case TGUI_CURSOR_H_ARROW: { XDefineCursor(d, window->os_window, cursor_h_double_arrow); } break;
    }
    
}

/* ---------------------
        OpenGL 
   --------------------- */

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
typedef void (*glXSwapIntervalEXTProc)(Display *, GLXDrawable , int);

#define X(return, name, params) TGUI_GL_PROC(name) name;
GL_FUNCTIONS(X)
#undef X

void os_gl_create_context(struct OsWindow *window) {
    Display *d = g_x11_display;
    
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
  
    int ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        None
    };
  
    /* TODO: Get context version to be sure we are in 3.3 version */
    window->ctx = glXCreateContextAttribsARB(d, window->best_fbc, 0, True, ctx_attribs);
    XSync(d, False);
    if(!glXIsDirect(d, window->ctx)) {
        printf("Indirect rendering not supported\n");
        return;
    }

    glXMakeCurrent(d, window->os_window, window->ctx); 
  
    /* Set vertical Sync */
    glXSwapIntervalEXTProc glXSwapIntervalEXT = (glXSwapIntervalEXTProc)glXGetProcAddress((GLubyte *)"glXSwapIntervalEXT");
    if(glXSwapIntervalEXT) {
        glXSwapIntervalEXT(d, window->os_window, 0);
        printf("Vertical Sync enable\n");
    } else {
        printf("vertical Sync disable\n");
    }

    /* TODO: load opnegl function: */
    #define X(return, name, params) name = (TGUI_GL_PROC(name))glXGetProcAddress((GLubyte *)#name);
    GL_FUNCTIONS(X)
    #undef X

}

void os_gl_destroy_context(struct OsWindow *window) {
    Display *d = g_x11_display;
    glXDestroyContext(d, window->ctx);
}

void os_gl_swap_buffers(struct OsWindow *window) {
    Display *d = g_x11_display;
    glXSwapBuffers(d, window->os_window);
}
/* -------------------------------------------- */
/*         TGui Backend implementation          */
/* -------------------------------------------- */



void *tgui_opengl_create_program(char *vert, char *frag) {
    
    OsFile *file_vert = os_file_read_entire(vert);
    OsFile *file_frag = os_file_read_entire(frag);
    
    char *v_src = file_vert->data;
    char *f_src = file_frag->data;

    int success;
    static char infoLog[512];

    unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, (const char **)&v_src, NULL);
    glCompileShader(v_shader);
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(v_shader, 512, NULL, infoLog);
        printf("[VERTEX SHADER ERROR]: %s\n", infoLog);
    }

    unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, (const char **)&f_src, NULL);
    glCompileShader(f_shader);
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(f_shader, 512, NULL, infoLog);
        printf("[FRAGMENT SHADER ERROR]: %s\n", infoLog);
    }
    
    tgui_u32 program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("[SHADER PROGRAM ERROR]: %s\n", infoLog);
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    os_file_free(file_vert);
    os_file_free(file_frag);

    return (void *)(tgui_u64)program;
}

void tgui_opengl_destroy_program(void *program) {
    glDeleteProgram((tgui_u64)program);
}

void *tgui_opengl_create_texture(tgui_u32 *data, tgui_u32 width, tgui_u32 height) {
    
    tgui_u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return (void *)(tgui_u64)texture;
}

void tgui_opengl_destroy_texture(void *texture) {
    tgui_u32 id = (tgui_u64)texture;
    glDeleteTextures(1, &id);
}

void tgui_opengl_set_program_width_and_height(void *program, tgui_u32 width, tgui_u32 height) {
    tgui_u32 id = (tgui_u64)program;
    glUseProgram(id);
    glUniform1i(glGetUniformLocation(id, "res_x"), width);
    glUniform1i(glGetUniformLocation(id, "res_y"), height);
}

tgui_u32 vao, vbo, ebo, rbo, fbo;
void *custom_program;
void *custom_texture;
#define MAX_QUAD_PER_BATCH 1024

void tgui_opengl_initialize_buffers(void) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);  

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUAD_PER_BATCH*(sizeof(TGuiVertex) * 4), 0, GL_DYNAMIC_DRAW); 
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), TGUI_OFFSET_OF(TGuiVertex, x)); 

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), TGUI_OFFSET_OF(TGuiVertex, u)); 

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), TGUI_OFFSET_OF(TGuiVertex, r)); 

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUAD_PER_BATCH*(sizeof(tgui_u32) * 6), 0, GL_DYNAMIC_DRAW); 
    
    /* ------------------------------------------------ */
    /*       FrameBuffer test                           */

    custom_program = tgui_opengl_create_program("./shaders/triangle.vert", "./shaders/triangle.frag");
    custom_texture = tgui_opengl_create_texture(NULL, 1024, 1024);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1024, 1024);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (tgui_u64)custom_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* ------------------------------------------------ */

}

void tgui_opengl_draw_buffers(void *program, void *texture, TGuiVertexArray *vertex_buffer, TGuiU32Array *index_buffer) {

        ASSERT(tgui_array_size(vertex_buffer) <= MAX_QUAD_PER_BATCH*4);
        ASSERT(tgui_array_size(index_buffer)  <= MAX_QUAD_PER_BATCH*6);
        
        tgui_u32 program_id = (tgui_u64)program;
        glUseProgram(program_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, tgui_array_size(vertex_buffer)*sizeof(TGuiVertex), tgui_array_data(vertex_buffer));
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tgui_array_size(index_buffer)*sizeof(tgui_u32), tgui_array_data(index_buffer));

        glBindVertexArray(vao);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

        tgui_u32 texture_id = (tgui_u64)texture;
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glDrawElements(GL_TRIANGLES, tgui_array_size(index_buffer), GL_UNSIGNED_INT, 0);

}
/* -------------------------------------------- */

void add_docker_nodes(TGuiDockerNode *node) {
    
    if(!node) return;

    switch (node->type) {
    
    case TGUI_DOCKER_NODE_ROOT:   { 
        _tgui_tree_view_root_node_begin("root node", node);
        TGuiDockerNode *child = node->childs->next;
        while(!tgui_clink_list_end(child, node->childs)) {
            add_docker_nodes(child);
            child = child->next;
        }
        _tgui_tree_view_root_node_end();
    } break;
    case TGUI_DOCKER_NODE_SPLIT:  {
        _tgui_tree_view_node("split node", node);
    } break;
    case TGUI_DOCKER_NODE_WINDOW: {
        _tgui_tree_view_root_node_begin("window node", node);
        TGuiWindow *window = node->windows->next; 
        while(!tgui_clink_list_end(window, node->windows)) {
            _tgui_tree_view_node(window->name, window);
            window = window->next;
        }

        _tgui_tree_view_root_node_end();
    } break;
    
    }
}

extern TGuiDocker docker;

int main(void) {

    os_initialize();
        
    OsWindow *window = os_window_create("TGUI sandbox", 10, 10, 1280, 720);
    
    os_gl_create_context(window);
    tgui_opengl_initialize_buffers();
    
    TGuiGfxBackend gfx;
    gfx.create_program               = tgui_opengl_create_program;
    gfx.destroy_program              = tgui_opengl_destroy_program;
    gfx.create_texture               = tgui_opengl_create_texture;
    gfx.destroy_texture              = tgui_opengl_destroy_texture;
    gfx.set_program_width_and_height = tgui_opengl_set_program_width_and_height;
    gfx.draw_buffers                 = tgui_opengl_draw_buffers;
    
    
    tgui_u64 miliseconds_per_frame = 16;
    tgui_f32 seconds_per_frame = (tgui_f32)miliseconds_per_frame / 1000.0f; (void)seconds_per_frame;
    tgui_u64 last_time = os_get_ticks();
    
    tgui_initialize(1280, 720, &gfx);
    
    /* NOTE: Load custom textures here */

    tgui_texture_atlas_generate_atlas();

    TGuiWindowHandle window0 = tgui_create_root_window("Window 0", false);
    TGuiWindowHandle window1 = tgui_split_window(window0, TGUI_SPLIT_DIR_HORIZONTAL, "Window 1", TGUI_WINDOW_TRANSPARENT);
    TGuiWindowHandle window2 = tgui_split_window(window0, TGUI_SPLIT_DIR_VERTICAL,   "Window 2", TGUI_WINDOW_SCROLLING);
    TGuiWindowHandle window3 = tgui_split_window(window1, TGUI_SPLIT_DIR_HORIZONTAL, "Window 3", TGUI_WINDOW_SCROLLING);
    TGuiWindowHandle window4 = tgui_split_window(window3, TGUI_SPLIT_DIR_VERTICAL,   "Window 4", TGUI_WINDOW_SCROLLING);
    TGUI_UNUSED(window4);
    
    tgui_try_to_load_data_file();

    char *options[] = {
        "option 0",
        "option 1",
        "option 2",
        "option 3",
        "option 4",
        "option 5",
        "option 6",
        "option 7",
        "option 8",
        "option 9",
    };
    tgui_s32 option_index;

    while(!window->should_close) {
    
        os_window_poll_events(window, tgui_get_input());
    
        /* NOTE: TGui code Here!!! */
        tgui_begin(miliseconds_per_frame);

        tgui_texture(window1, custom_texture);

        if(tgui_button(window0, "button", 10, 10)) {
            printf("click! 0\n");
        }

        if(tgui_button(window0, "button", 10, 60)) {
            printf("click! 1\n");
        }
        if(tgui_button(window3, "button", 10, 10)) {
            printf("click! 2\n");
        }
        if(tgui_button(window3, "button", 160, 10)) {
            printf("click! 3\n");
        }
        if(tgui_button(window3, "button", 310, 10)) {
            printf("click! 4\n");
        }

        tgui_text_input(window2, 10, 10);

        tgui_text_input(window2, 180, 10);
        
        if(tgui_button(window1, "button", 10, 10)) {
            printf("click! 5\n");
        }
        
        void *user_data = NULL;
        _tgui_tree_view_begin(window4, TGUI_ID);

            add_docker_nodes(docker.root);
        
        _tgui_tree_view_end(&user_data);

        if(tgui_button(window2, "button", 10, 100)) {
            printf("click! 6\n");
        }
        
        tgui_dropdown_menu(window2, 10, 60, options, ARRAY_LEN(options), &option_index);
        
        tgui_dropdown_menu(window2, 180, 60, options, ARRAY_LEN(options), &option_index);

        tgui_end();

        /* ------------------------------------------------ */

        /* NOTE: Rendering code Here!!! */

        glViewport(0, 0, window->width, window->height);

        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
         
        tgui_draw_buffers();


        /* ------------------------------------------------ */
        /*       FrameBuffer test                           */

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, 1024, 1024);

        glUseProgram((tgui_u64)custom_program);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        TGuiVertex tri_vertices[3] = {
            {-0.5f, -0.5f,   0, 0,  1,0,0}, 
            { 0.5f, -0.5f,   0, 1,  0,1,0}, 
            { 0.0f,  0.5f,   1, 1,  0,0,1}, 
        };
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 3*sizeof(TGuiVertex), tri_vertices);
        glDrawArrays(GL_TRIANGLES, 0, 3);
         
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        /* ------------------------------------------------ */


        os_gl_swap_buffers(window);

        /* ------------------------------------------------ */

        tgui_u64 current_time = os_get_ticks();
        tgui_u64 delta_time = current_time - last_time; (void)delta_time;
        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        last_time = current_time; (void)last_time; 

    }

    tgui_terminate();

    os_gl_destroy_context(window);

    os_terminate();

    printf("Application close perfectly\n");

    return 0;
}
