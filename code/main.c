#include "common.h"
#include "geometry.h"
#include "os.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"

#include "os.c"
#include "geometry.c"
#include "memory.c"
#include "painter.c"
#include "tgui.c"
#include "tgui_docker.c"

#include "app.c"

static struct OsBackbuffer *realloc_backbuffer(struct OsBackbuffer *backbuffer, struct OsWindow *window, s32 w, s32 h) {
    
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
    struct OsBackbuffer *backbuffer = realloc_backbuffer(0, window, window_w, window_h);
   
    u64 miliseconds_per_frame = 16;
    f32 seconds_per_frame = (f32)miliseconds_per_frame / 1000.0f;
    u64 last_time = os_get_ticks();
     

    TGuiInput *input = tgui_get_input();
    
    input->window_resize = true;
    input->resize_w = window_w;
    input->resize_h = window_h;
    
    App app;
    app_initialize(&app);
    
    painter_initialize(&arena);

    while(!os_window_should_close(window)) {

        os_window_poll_events(window);
        
        if(os_window_resize(window)) {

            window_w = os_window_width(window);
            window_h = os_window_height(window);
            backbuffer = realloc_backbuffer(backbuffer, window, window_w, window_h);
            input->window_resize = true;
            input->resize_w = window_w;
            input->resize_h = window_h;
        }

        os_window_get_mouse_position(window, &input->mouse_x, &input->mouse_y);
        os_window_get_mouse_lbutton_state(window, &input->mouse_button_is_down);


        switch (tgui_get_cursor_state()) { 
        
        case TGUI_CURSOR_ARROW:   { os_set_cursor(window, OS_CURSOR_ARROW);   } break;
        case TGUI_CURSOR_HAND:    { os_set_cursor(window, OS_CURSOR_HAND);    } break;
        case TGUI_CURSOR_V_ARROW: { os_set_cursor(window, OS_CURSOR_V_ARROW); } break;
        case TGUI_CURSOR_H_ARROW: { os_set_cursor(window, OS_CURSOR_H_ARROW); } break;

        }

        Painter painter;
        painter_start(&painter, (u32 *)backbuffer->data, (Rectangle){0, 0, window_w-1, window_h-1}, 0);

        app_draw(&app, &painter);
        app_update(&app, seconds_per_frame, &painter);
        
#if 0
        /* NOTE: test draw test */
        s32 test_pos_x = 40;
        s32 test_pos_y = 100;
        char *test = "Tomas Cabrerizo";
        Rectangle text_rect = painter_get_text_dim(&painter, test_pos_x, test_pos_y, test);
        painter_draw_rectangle(&painter, text_rect, 0x000000);
        painter_draw_text(&painter, test_pos_x, test_pos_y, test, 0xa6edaf);
#endif

        input->mouse_button_was_down = input->mouse_button_is_down;
        
        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time;

        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        
        last_time = current_time; 
        
        os_backbuffer_swap(window, backbuffer);
        //printf("ms: %lld\n", delta_time);
        
    }
    
    painter_terminate();

    app_terminate(&app);

    os_backbuffer_destroy(backbuffer);
    os_window_destroy(window);
    
    arena_terminate(&arena);
    os_terminate();

    printf("Program exit correctly!\n");

    return 0;
}
