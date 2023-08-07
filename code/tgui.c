#include "tgui.h"
#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"
#include "tgui_docker.h"

/* -------------------------- */
/*       Hash Function     */
/* -------------------------- */

 static u64 murmur_hash64A(const void * key, int len, u64 seed) {
        const u64 m = 0xc6a4a7935bd1e995ull;
        const int r = 47;
        u64 h = seed ^ (len * m);
        
        const u64 * data = (const u64 *)key;
        const u64 * end = data + (len/8);
        
        while(data != end) {
            u64 k = *data++;

            k *= m; 
            k ^= k >> r; 
            k *= m; 
    
            h ^= k;
            h *= m; 
        }

        const unsigned char * data2 = (const unsigned char*)data;

        switch(len & 7) {
            case 7: h ^= (u64)(data2[6]) << 48;
            case 6: h ^= (u64)(data2[5]) << 40;
            case 5: h ^= (u64)(data2[4]) << 32;
            case 4: h ^= (u64)(data2[3]) << 24;
            case 3: h ^= (u64)(data2[2]) << 16;
            case 2: h ^= (u64)(data2[1]) << 8;
            case 1: h ^= (u64)(data2[0]);
                h *= m;
        };
 
        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
} 

TGui state;
TGuiInput input;
extern TGuiDocker docker;

void tgui_initialize(void) {
    
    memset(&state, 0, sizeof(TGui));

    arena_initialize(&state.arena, 0, ARENA_TYPE_VIRTUAL);
    virtual_map_initialize(&state.registry);
    
    tgui_docker_initialize();
}

void tgui_terminate(void) {
    
    tgui_docker_terminate();

    virtual_map_terminate(&state.registry);
    arena_terminate(&state.arena);
    memset(&state, 0, sizeof(TGui));
}


void tgui_update(void) {

    tgui_docker_update();
 
    input.mouse_button_was_down = input.mouse_button_is_down;

}

TGuiInput *tgui_get_input(void) {
    return &input;
}


TGuiCursor tgui_get_cursor_state(void) {
    return state.cursor;
}

u64 tgui_hash(void *bytes, u64 size) {
    return murmur_hash64A(bytes, size, 123);
}

/* -------------------------- */
/*       Widgets Function     */
/* -------------------------- */

static Rectangle calculate_buttom_rect(TGuiDockerNode *window, s32 x, s32 y) {
    Rectangle button_rect = (Rectangle){0, 0, 100, 30};
    Rectangle window_rect = tgui_docker_get_client_rect(window);
    button_rect.min_x = window_rect.min_x + button_rect.min_x + x;
    button_rect.min_y = window_rect.min_y + button_rect.min_y + y;
    button_rect.max_x = button_rect.min_x + button_rect.max_x;
    button_rect.max_y = button_rect.min_y + button_rect.max_y;
    return rect_intersection(button_rect, window_rect);
}

b32 tgui_button(struct TGuiDockerNode *window, const char *label, s32 x, s32 y, Painter *painter) {
    
    if(!tgui_docker_window_is_visible(window)) {
        return false;
    }

    Rectangle saved_painter_clip = painter->clip;
    painter->clip = tgui_docker_get_client_rect(window);

    u64 id = tgui_hash((void *)label, strlen(label));
    Rectangle buttom_rect = calculate_buttom_rect(window, x, y); 
    
    u32 button_color = 0xcccccc;

    if(rect_point_overlaps(buttom_rect, input.mouse_x, input.mouse_y)) {
        state.hot = id;
        button_color = 0xbbbbbb;
    }
    
    /* TODO: Desing a command interface to render the complete UI independently */
    painter_draw_rectangle(painter, buttom_rect, button_color);
    
    painter->clip = saved_painter_clip;

    return false;
}

