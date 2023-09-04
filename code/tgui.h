#ifndef _TGUI_H_
#define _TGUI_H_

#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"
#include "tgui_docker.h"
#include "tgui_gfx.h"

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

/* ---------------------- */
/*       TGui Window      */
/* ---------------------- */

#define TGUI_SCROLL_BAR_SIZE 20

struct TGuiAllocatedWindow;

typedef struct TGuiWindow {
    
    u32 id;
    Rectangle dim;
    char *name;
    
    struct TGuiWidget *widgets;
    
    b32 is_scrolling;
    Rectangle h_scroll_bar;
    Rectangle v_scroll_bar;
    
    f32 v_scroll_offset;
    f32 h_scroll_offset;
    
    b32 v_scroll_active;
    b32 h_scroll_active;
    
    Rectangle scroll_saved_rect;

    struct TGuiDockerNode *parent;
    struct TGuiWindow *next;
    struct TGuiWindow *prev;

} TGuiWindow;

typedef u32 TGuiWindowHandle;

TGuiWindow *tgui_window_alloc(TGuiDockerNode *parent, char *name, b32 scroll, struct TGuiAllocatedWindow *list);

TGuiWindow *tgui_window_get_from_handle(TGuiWindowHandle window);

TGuiWindowHandle tgui_create_root_window(char *name, b32 scroll);

TGuiWindowHandle tgui_split_window(TGuiWindowHandle window, TGuiSplitDirection dir, char *name, b32 scroll);

typedef void (*TGuiWidgetInternalFunc) (struct TGuiWidget *widget, Painter *painter);

typedef struct TGuiWidget {
    u64 id;
    s32 x, y, w, h;    

    struct TGuiWindow *parent;
    struct TGuiWidget *prev;
    struct TGuiWidget *next;
    
    TGuiWidgetInternalFunc internal;
    
} TGuiWidget;

typedef struct TGuiAllocatedWindow {
    TGuiWindow window;
    struct TGuiAllocatedWindow *next;
    struct TGuiAllocatedWindow *prev;
} TGuiAllocatedWindow;

TGuiAllocatedWindow *tgui_allocated_window_node_alloc(void);

void tgui_allocated_window_node_free(TGuiAllocatedWindow *allocated_window);

#define TGUI_MAX_WINDOW_REGISTRY 256

typedef struct TGui {

    TGuiCursor cursor;
    
    Arena arena;
    VirtualMap registry;
    
    u32 window_id_generator;

    TGuiAllocatedWindow *allocated_windows;
    TGuiAllocatedWindow *free_windows;

    u64 hot;
    u64 active;
    
    f32 dt;
    
    TGuiWidget *first_free_widget;
    
    u64 active_id;
    TGuiWindow *active_window;

    TGuiTextureAtlas texture_atlas;

} TGui;

void tgui_initialize(s32 w, s32 h);

void tgui_terminate(void);

void tgui_begin(f32 dt);

void tgui_end(Painter *painter);

void tgui_try_to_load_data_file(void);

void tgui_free_allocated_windows_list(struct TGuiAllocatedWindow *list);

TGuiInput *tgui_get_input(void);

TGuiCursor tgui_get_cursor_state(void);

TGuiTextureAtlas *tgui_get_texture_atlas(void);

#define tgui_safe_dereference(ptr, type) (((ptr) == NULL) ? (type){0} : *((type *)ptr))

/* ---------------------- */
/*       TGui Widgets     */
/* ---------------------- */

#define tgui_widget_get_state(id, type) (type*)_tgui_widget_get_state((id), sizeof(type))

#define tgui_button(window, label, x, y) _tgui_button((window), (label), (x), (y), TGUI_ID)

#define tgui_text_input(window, x, y) _tgui_text_input((window), (x), (y), TGUI_ID)

#define tgui_drop_down_menu_begin(window, x, y, painter) _tgui_drop_down_menu_begin((window), (x), (y), (painter), TGUI_ID)

#define tgui_drop_down_menu_item(window, label, painter) _tgui_drop_down_menu_item((window), (label), (painter))

#define tgui_drop_down_menu_end(window, painter) _tgui_drop_down_menu_end((window), (painter));

#define tgui_color_picker(window, x, y, w, h, color) _tgui_color_picker((window), (x), (y), (w), (h), (color), TGUI_ID)

typedef struct TGuiButton {
    char *label;
    b32 result;
} TGuiButton;

b32 _tgui_button(TGuiWindowHandle window, char *label, s32 x, s32 y, char *tgui_id);

void _tgui_button_internal(TGuiWidget *widget, Painter *painter);

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

TGuiTextInput *_tgui_text_input(TGuiWindowHandle window, s32 x, s32 y, char *tgui_id);

void _tgui_text_input_internal(TGuiWidget *widget, Painter *painter);

typedef struct TGuiColorPicker {
    
    TGuiBitmap radiant;
    TGuiBitmap mini_radiant;
    
    u32 saved_radiant_color;

    f32 hue;
    f32 saturation;
    f32 value;
    
    b32 hue_cursor_active;
    b32 sv_cursor_active;

    u32 *color_ptr;

    b32 initialize;

} TGuiColorPicker;

void tgui_u32_color_to_hsv_color(u32 color, f32 *h, f32 *s, f32 *v);

void _tgui_color_picker(TGuiWindowHandle window, s32 x, s32 y, s32 w, s32 h, u32 *color, char *tgui_id);

void _tgui_color_picker_internal(TGuiWidget *widget, Painter *painter);

typedef struct TGuiTreeViewNode {
        
    char *label;
    u32 label_depth;
    
    Rectangle dim;
    
    u32 state_index;
    b32 visible;
    
    b32 selected;
    u32 selected_state_index;
    
    void *user_data;

    struct TGuiTreeViewNode *next;
    struct TGuiTreeViewNode *prev;

    struct TGuiTreeViewNode *parent;
    struct TGuiTreeViewNode *childs;

} TGuiTreeViewNode;

#define TGUI_TREEVIEW_MAX_STATE_SIZE 256

#define TGUI_TREEVIEW_COLOR0 0x888888
#define TGUI_TREEVIEW_COLOR1 0x999999

#define TGUI_TREEVIEW_DEFAULT_DEPTH_WIDTH 16 

typedef struct TGuiTreeView {
    
    TGuiTreeViewNode *root;
    TGuiTreeViewNode *active_root_node;
    s32 active_depth;
    
    TGuiTreeViewNode *first_free_node;
    
    Rectangle dim;
    
    /* TODO: Make this two arrays dynamic or use hash table or link list */

    b32 root_node_state[TGUI_TREEVIEW_MAX_STATE_SIZE];
    u32 root_node_state_head; 

    b32 selected_node_data[TGUI_TREEVIEW_MAX_STATE_SIZE];
    u32 selected_node_data_head;
   
    u32 rect_w;
    u32 padding;

    b32 initiliaze;

    s32 selection_index;
    void *selection_data;

} TGuiTreeView;

void _tgui_tree_view_begin(TGuiWindowHandle window, char *tgui_id);

void _tgui_tree_view_end(void **selected_data);

void _tgui_tree_view_root_node_begin(char *label, void *user_data);

void _tgui_tree_view_root_node_end(void);

void _tgui_tree_view_node(char *label, void *user_data);

void _tgui_tree_view_internal(TGuiWidget *widget, Painter *painter);

#define TGUI_DROPDOWN_MENU_DELFAUT_W 140
#define TGUI_DROPDOWN_MENU_DELFAUT_H 26 

typedef struct TGuiDropDownMenu {
    
    char **options;
    u32 options_size;
    u32 selected_option;
    
    b32 click_was_in_scrollbar;

    b32 initialize;

} TGuiDropDownMenu;

void _tgui_dropdown_menu(TGuiWindowHandle window, s32 x, s32 y, char **options, u32 options_size, s32 *selected_option_index, char *tgui_id);

void _tgui_dropdown_menu_internal(TGuiWidget *widget, Painter *painter);

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
