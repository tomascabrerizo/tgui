#include "tgui.h"
#include "common.h"
#include "memory.h"
#include "painter.h"
#include "tgui_docker.h"
#include <stdio.h>

u64 hash(u64 key) {
    key = (~key) + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8);
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
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

