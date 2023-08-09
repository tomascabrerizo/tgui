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
    Rectangle button_rect = (Rectangle){0, 0, 120, 30};
    Rectangle window_rect = tgui_docker_get_client_rect(window);
    button_rect.min_x = window_rect.min_x + button_rect.min_x + x;
    button_rect.min_y = window_rect.min_y + button_rect.min_y + y;
    button_rect.max_x = button_rect.min_x + button_rect.max_x;
    button_rect.max_y = button_rect.min_y + button_rect.max_y;
    return rect_intersection(button_rect, window_rect);
}

b32 tgui_button(struct TGuiDockerNode *window, char *label, s32 x, s32 y, Painter *painter) {
    
    if(!tgui_docker_window_is_visible(window)) {
        return false;
    }
    
    b32 result = false;

    u64 id = tgui_hash((void *)label, strlen(label));
    Rectangle button_rect = calculate_buttom_rect(window, x, y); 
    
    Rectangle saved_painter_clip = painter->clip;
    painter->clip = button_rect;
    
    u32 button_color = 0xaaaaaa;
    b32 mouse_is_over = rect_point_overlaps(button_rect, input.mouse_x, input.mouse_y);
    
    if(state.active == id) {
        if(!input.mouse_button_is_down && input.mouse_button_was_down) {
            if(state.hot == id) result = true;
            state.active = 0;
        }
    } else if(state.hot == id) {
        if(input.mouse_button_is_down) state.active = id;
    }

    if(mouse_is_over && !state.active) {
        state.hot = id;
        button_color = 0xbbbbbb;
    }

    if(!mouse_is_over && state.hot == id) {
        state.hot = 0;
    }
     
    if(result) button_color = 0x00ff00;


    /* TODO: Desing a command interface to render the complete UI independently */
    painter_draw_rectangle(painter, button_rect, button_color);

    Rectangle label_rect = painter_get_text_dim(painter, 0, 0, label);
    
    s32 label_x = button_rect.min_x + rect_width(button_rect) / 2 - rect_width(label_rect) / 2;
    s32 label_y = button_rect.min_y + rect_height(button_rect) / 2 - rect_height(label_rect) / 2;
    
    u32 decoration_color = 0x444444;
    painter_draw_text(painter, label_x, label_y, label,  decoration_color);
    painter_draw_rectangle_outline(painter, button_rect, decoration_color);

    painter->clip = saved_painter_clip;

    return result;
}

