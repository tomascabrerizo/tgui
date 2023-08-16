#ifndef _TGUI_H_
#define _TGUI_H_

#include "common.h"
#include "memory.h"
#include "painter.h"

/* TODO: Create actual TGuiWindow instead of using the TGuiDockerNode */
struct TGuiDockerNode;

typedef enum TGuiCursor {

    TGUI_CURSOR_ARROW, 
    TGUI_CURSOR_HAND,
    TGUI_CURSOR_V_ARROW, 
    TGUI_CURSOR_H_ARROW 

} TGuiCursor;

#define TGUI_MAX_TEXT_SIZE 32

typedef struct TGuiKeyboard {

    b32 k_r_arrow_down;
    b32 k_l_arrow_down;
    b32 k_delete;
    b32 k_backspace;
    b32 k_ctrl;
    b32 k_shift;

} TGuiKeyboard;

typedef struct TGuiInput {
    b32 mouse_button_is_down;
    b32 mouse_button_was_down;
    
    s32 mouse_x;
    s32 mouse_y;
    
    b32 window_resize;
    s32 resize_w;
    s32 resize_h;
    
    u8 text[TGUI_MAX_TEXT_SIZE];
    u32 text_size;
    
    TGuiKeyboard keyboard;
 
} TGuiInput;

typedef struct TGui {

    TGuiCursor cursor;

    Arena arena;
    VirtualMap registry;

    u64 hot;
    u64 active;

} TGui;

void tgui_initialize(void);

void tgui_terminate(void);

void tgui_update(void);

TGuiInput *tgui_get_input(void);

TGuiCursor tgui_get_cursor_state(void);

/* ---------------------- */
/*       TGui Widgets     */
/* ---------------------- */

b32 tgui_button(struct TGuiDockerNode *window, char *label, s32 x, s32 y, Painter *painter);

#define TGUI_TEXT_INPUT_MAX_CHARACTERS 124 
typedef struct TGuiTextInput {
    u8 buffer[TGUI_TEXT_INPUT_MAX_CHARACTERS];
    u32 used;
    u32 cursor;
    
    u32 offset;
    
    b32 initilize;
    
    u32 font_width;
    u32 font_height;
    
    b32 selection;
    u32 selection_start;
    u32 selection_end;

} TGuiTextInput;

TGuiTextInput *tgui_text_input(struct TGuiDockerNode *window, s32 x, s32 y, char *label, Painter *painter);

#endif /* _TGUI_H_ */
