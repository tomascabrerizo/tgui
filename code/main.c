#include "common.h"
#include "geometry.h"
#include "os.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"
#include "tgui_serializer.h"

#include "os.c"
#include "geometry.c"
#include "memory.c"
#include "tgui.c"
#include "tgui_docker.c"
#include "tgui_serializer.c"
#include <GL/gl.h>

#define HARWARE_RENDERING 1
#if HARWARE_RENDERING
#include "painter_hardware.c"
#else
#include "painter.c"
#endif


#include "app.c"
#include <stdio.h>

struct OsBackbuffer *realloc_backbuffer(struct OsBackbuffer *backbuffer, struct OsWindow *window, s32 w, s32 h) {
    
    static Arena arena;
    static b32 arena_is_initialize = false;
    if(!arena_is_initialize) {
        arena_initialize(&arena, 0, ARENA_TYPE_VIRTUAL);
        arena_is_initialize = true;
    }
    
    if(!backbuffer) {
        u32 *pixels = (u32 *)arena_alloc(&arena, w*h*sizeof(u32), 8);
        backbuffer = os_backbuffer_create(&arena, window, pixels, w, h, sizeof(u32));
    } else {
        os_backbuffer_destroy(backbuffer);
        arena_free(&arena);
        u32 *pixels = (u32 *)arena_alloc(&arena, w*h*sizeof(u32), 8);
        backbuffer = os_backbuffer_create(&arena, window, pixels, w, h, sizeof(u32));
    }

    return backbuffer;
}

int main(void) {

    os_initialize();
    
    Arena arena;
    arena_initialize(&arena, 0, ARENA_TYPE_VIRTUAL);
    
    
    s32 window_w = 800;
    s32 window_h = 600;
    struct OsWindow *window = os_window_create(&arena, "Test Window", 0, 0, window_w, window_h);

#if HARWARE_RENDERING
    os_gl_create_context(window);

    TGuiVertexArray vertex_array;
    TGuiU32Array  index_array;

    tgui_array_initialize(&vertex_array);
    tgui_array_initialize(&index_array);
#else
    struct OsBackbuffer *backbuffer = realloc_backbuffer(0, window, window_w, window_h);
#endif

    u64 miliseconds_per_frame = 16;
    f32 seconds_per_frame = (f32)miliseconds_per_frame / 1000.0f; (void)seconds_per_frame;
    u64 last_time = os_get_ticks();
     

    TGuiInput *input = tgui_get_input();

    App app;
    app_initialize(&app, window_w, window_h);
    
    while(!os_window_should_close(window)) {
        
        os_window_poll_events(window);
        
        if(os_window_resize(window)) {

            window_w = os_window_width(window);
            window_h = os_window_height(window);
            
            if(window_w <= 1) {
                u32 break_here = 0;
                (void)break_here;
            }
#if HARWARE_RENDERING
            glViewport(0, 0, window_w, window_h);
#else
            backbuffer = realloc_backbuffer(backbuffer, window, window_w, window_h);
#endif
            input->window_resize = true;
            input->resize_w = window_w;
            input->resize_h = window_h;
        }

        os_window_get_mouse_position(window, &input->mouse_x, &input->mouse_y);
        os_window_get_mouse_lbutton_state(window, &input->mouse_button_is_down);
        os_window_get_text_input(window, input->text, &input->text_size, TGUI_MAX_TEXT_SIZE);
        os_window_get_keyboard_input(window, &input->keyboard);


        switch (tgui_get_cursor_state()) { 
        
        case TGUI_CURSOR_ARROW:   { os_set_cursor(window, OS_CURSOR_ARROW);   } break;
        case TGUI_CURSOR_HAND:    { os_set_cursor(window, OS_CURSOR_HAND);    } break;
        case TGUI_CURSOR_V_ARROW: { os_set_cursor(window, OS_CURSOR_V_ARROW); } break;
        case TGUI_CURSOR_H_ARROW: { os_set_cursor(window, OS_CURSOR_H_ARROW); } break;

        }
        
        //printf("vertex array size: %lld, capacity: %lld, arena used: %lld, arena size: %lld\n", tgui_array_size(&vertex_array), vertex_array.type_array.capacity, vertex_array.type_array.arena.used, vertex_array.type_array.arena.size);
        //printf("index array size: %lld, capacity: %lld, arena used: %lld, arena size: %lld\n", tgui_array_size(&index_array), index_array.type_array.capacity, index_array.type_array.arena.used, index_array.type_array.arena.size);

        Painter painter;
#if HARWARE_RENDERING
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        painter_start(&painter, (Rectangle){0, 0, window_w-1, window_h-1}, 0, NULL, &vertex_array, &index_array);
#else
        painter_start(&painter, (Rectangle){0, 0, window_w-1, window_h-1}, 0, (u32 *)backbuffer->data, NULL, NULL);
#endif

        painter_clear(&painter, 0x00);

        app_update(&app, seconds_per_frame, &painter);

#if HARWARE_RENDERING
        glDrawElements(GL_TRIANGLES, tgui_array_size(&vertex_array), GL_UNSIGNED_INT, tgui_array_data(&index_array));
#endif

        input->mouse_button_was_down = input->mouse_button_is_down;
        
        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time; (void)delta_time;
#if HARWARE_RENDERING == 0
        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
#endif
        last_time = current_time; (void)last_time; 
        

#if HARWARE_RENDERING
        os_gl_swap_buffers(window);
#else
        os_backbuffer_swap(window, backbuffer);
#endif
        //printf("ms: %lld\n", delta_time);
        
    }
    
    app_terminate(&app);

#if HARWARE_RENDERING
    os_gl_destroy_context(window);
#else
    os_backbuffer_destroy(backbuffer);
#endif

    os_window_destroy(window);
    
    arena_terminate(&arena);
    os_terminate();

    printf("Program exit correctly!\n");

    return 0;
}
