#include "common.h"
#include "os.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"

#include "os.c"
#include "memory.c"
#include "painter.c"
#include "tgui.c"
#include "tgui_docker.c"

#include "app.c"

void clear_backbuffer(OsBackbuffer *backbuffer) {
    memset(backbuffer->data, 0, backbuffer->width*backbuffer->height*backbuffer->bytes_per_pixel);
}

static OsBackbuffer *realloc_backbuffer(OsBackbuffer *backbuffer, OsWindow *window, s32 w, s32 h) {
    
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
    OsWindow *window = os_window_create(&arena, "Test Window", 0, 0, window_w, window_h);
    OsBackbuffer *backbuffer = realloc_backbuffer(0, window, window_w, window_h);
   
    u64 miliseconds_per_frame = 16;
    u64 last_time = os_get_ticks();
    
    tgui_initialize();
    
    TGuiDockerNode *window_node0 = window_node_alloc(0);
    TGuiDockerNode *window_node1 = window_node_alloc(0);
    TGuiDockerNode *window_node2 = window_node_alloc(0);
    TGuiDockerNode *window_node3 = window_node_alloc(0);
    TGuiDockerNode *window_node4 = window_node_alloc(0);
    TGuiDockerNode *window_node5 = window_node_alloc(0);

    tgui_docker_set_root_node(window_node0);
    tgui_docker_node_split(window_node0, TGUI_SPLIT_DIR_VERTICAL, window_node1);
    tgui_docker_node_split(window_node0, TGUI_SPLIT_DIR_VERTICAL, window_node2);
    tgui_docker_node_split(window_node2, TGUI_SPLIT_DIR_HORIZONTAL, window_node3);
    tgui_docker_node_split(window_node2, TGUI_SPLIT_DIR_HORIZONTAL, window_node4);
    tgui_docker_node_split(window_node1, TGUI_SPLIT_DIR_HORIZONTAL, window_node5);

    TGuiInput *input = tgui_get_input();
    
    input->window_resize = true;
    input->resize_w = window_w;
    input->resize_h = window_h;

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

        tgui_update();

        switch (tgui_get_cursor_state()) { 
        
        case TGUI_CURSOR_ARROW:   { os_set_cursor(window, OS_CURSOR_ARROW);   } break;
        case TGUI_CURSOR_HAND:    { os_set_cursor(window, OS_CURSOR_HAND);    } break;
        case TGUI_CURSOR_V_ARROW: { os_set_cursor(window, OS_CURSOR_V_ARROW); } break;
        case TGUI_CURSOR_H_ARROW: { os_set_cursor(window, OS_CURSOR_H_ARROW); } break;

        }

        clear_backbuffer(backbuffer);
        
        Rectangle dim = {0, 0, window_w, window_h};
        Painter painter;
        painter_initialize(&painter, (u32 *)backbuffer->data, dim, 0);
        
        root_node_draw(&painter);
        //node_draw(&painter, window_node0);
         
        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time;

        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        
        last_time = current_time; 
        
        os_backbuffer_swap(window, backbuffer);
        
        //printf("miliseconds per frame: %lld\n", delta_time);

    }
    
    tgui_terminate();

    os_backbuffer_destroy(backbuffer);
    os_window_destroy(window);
    
    arena_terminate(&arena);
    os_terminate();

    printf("Program exit correctly!\n");

    return 0;
}
