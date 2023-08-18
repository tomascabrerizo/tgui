#ifndef _TGUI_H_
#define _TGUI_H_

#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"
#include "tgui_docker.h"

#define TGUI_ID3(a, b) a#b
#define TGUI_ID2(a, b) TGUI_ID3(a, b)
#define TGUI_ID TGUI_ID2(__FILE__, __LINE__)

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

typedef struct TGuiBitmap {
    u32 *pixels;
    u32 width, height;
} TGuiBitmap;

typedef struct TGui {

    TGuiCursor cursor;

    Arena arena;
    VirtualMap registry;

    u64 hot;
    u64 active;
    
    f32 dt;
    void *active_data;

} TGui;

void tgui_initialize(void);

void tgui_terminate(void);

void tgui_update(f32 dt);

TGuiInput *tgui_get_input(void);

TGuiCursor tgui_get_cursor_state(void);

/* ---------------------- */
/*       TGui Widgets     */
/* ---------------------- */

#define tgui_widget_get_state(id, type) (type*)_tgui_widget_get_state((id), sizeof(type))

#define tgui_button(window, label, x, y, painter) _tgui_button(window, label, x, y, painter, TGUI_ID)
#define tgui_text_input(window, x, y, painter) _tgui_text_input(window, x, y, painter, TGUI_ID)

b32 _tgui_button(struct TGuiDockerNode *window, char *label, s32 x, s32 y, Painter *painter, char *tgui_id);

#define TGUI_TEXT_INPUT_MAX_CHARACTERS 124 
typedef struct TGuiTextInput {
    u8 buffer[TGUI_TEXT_INPUT_MAX_CHARACTERS];
    u32 used;
    u32 cursor;
    
    u32 offset;
    
    b32 initilize;
    
    b32 selection;
    u32 selection_start;
    u32 selection_end;
    
    f32 cursor_inactive_acumulator;
    f32 cursor_inactive_target;
    f32 cursor_blink_acumulator;
    f32 cursor_blink_target;
    b32 blink_cursor;
    b32 draw_cursor;

} TGuiTextInput;

TGuiTextInput *_tgui_text_input(struct TGuiDockerNode *window, s32 x, s32 y, Painter *painter, char *tgui_id);

typedef struct TGuiDropMenu {
    Rectangle rect;
    Rectangle parent_rect;

    b32 active;
    s32 selected_item;
    s32 running_index;

    Rectangle saved_clip;

    b32 initlialize;
} TGuiDropMenu;

void _tgui_drop_down_menu_begin(struct TGuiDockerNode *window, s32 x, s32 y, Painter *painter, char *tgui_id);

b32 _tgui_drop_down_menu_item(char *label, Painter *painter);

void _tgui_drop_down_menu_end(Painter *painter);

typedef struct TGuiColorPicker {
    
    TGuiBitmap radiant;
    TGuiBitmap mini_radiant;
    
    u32 selected_color;

    u32 selected_x;
    u32 selected_y;
    
    b32 initialize;

} TGuiColorPicker;

u32 _tgui_color_picker(struct TGuiDockerNode *window, s32 x, s32 y, s32 w, s32 h, Painter *painter, char *tgui_id);

/* ---------------------- */
/*       TGui Font        */
/* ---------------------- */

typedef struct TGuiGlyph {
    TGuiBitmap bitmap;
    s32 codepoint;

    s32 top_bearing;
    s32 left_bearing;
    s32 adv_width;

} TGuiGlyph;

typedef struct TGuiFont {
    TGuiGlyph *glyphs;
    
    u32 glyph_count;
    u32 glyph_rage_start;
    u32 glyph_rage_end;

    s32 ascent;
    s32 descent;
    s32 line_gap;

    u32 max_glyph_height;
    u32 max_glyph_width;
    
    struct OsFont *font;
} TGuiFont;

void tgui_font_initilize(Arena *arena);

void tgui_font_terminate(void);

TGuiGlyph *tgui_font_get_codepoint_glyph(u32 codepoint);

Rectangle tgui_get_size_text_dim(s32 x, s32 y, char *text, u32 size);

Rectangle tgui_get_text_dim(s32 x, s32 y, char *text);

void tgui_font_draw_text(Painter *painter, s32 x, s32 y, char *text, u32 size, u32 color);

#endif /* _TGUI_H_ */
