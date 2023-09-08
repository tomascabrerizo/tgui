#ifndef _TGUI_H_
#define _TGUI_H_

#include "tgui_docker.h"
#include "tgui_gfx.h"

#define TGUI_ID3(a, b) a#b
#define TGUI_ID2(a, b) TGUI_ID3(a, b)
#define TGUI_ID TGUI_ID2(__FILE__, __LINE__)

struct TGuiDockerNode;

typedef enum TGuiCursor {

    TGUI_CURSOR_ARROW, 
    TGUI_CURSOR_HAND,
    TGUI_CURSOR_V_ARROW, 
    TGUI_CURSOR_H_ARROW 

} TGuiCursor;

#define TGUI_MAX_TEXT_SIZE 32

typedef struct TGuiKeyboard {

    tgui_b32 k_r_arrow_down;
    tgui_b32 k_l_arrow_down;
    tgui_b32 k_delete;
    tgui_b32 k_backspace;
    tgui_b32 k_ctrl;
    tgui_b32 k_shift;

} TGuiKeyboard;

typedef struct TGuiInput {
    tgui_b32 mouse_button_is_down;
    tgui_b32 mouse_button_was_down;
    
    tgui_s32 mouse_x;
    tgui_s32 mouse_y;
    
    tgui_b32 window_resize;
    tgui_s32 resize_w;
    tgui_s32 resize_h;
    
    tgui_u8 text[TGUI_MAX_TEXT_SIZE];
    tgui_u32 text_size;
    
    TGuiKeyboard keyboard;
 
} TGuiInput;

/* ---------------------- */
/*       TGui Window      */
/* ---------------------- */

#define TGUI_SCROLL_BAR_SIZE 20

struct TGuiAllocatedWindow;

typedef enum TGuiWindowFlags {
    TGUI_WINDOW_SCROLLING   = 1 << 0,
    TGUI_WINDOW_TRANSPARENT = 1 << 1,
} TGuiWindowFlags;

typedef struct TGuiWindow {
    
    tgui_u32 id;
    TGuiRectangle dim;
    char *name;
    
    struct TGuiWidget *widgets;
    
    TGuiWindowFlags flags;

    TGuiRectangle h_scroll_bar;
    TGuiRectangle v_scroll_bar;
    
    tgui_f32 v_scroll_offset;
    tgui_f32 h_scroll_offset;
    
    tgui_b32 v_scroll_active;
    tgui_b32 h_scroll_active;
    
    TGuiRectangle scroll_saved_rect;

    struct TGuiDockerNode *parent;
    struct TGuiWindow *next;
    struct TGuiWindow *prev;

} TGuiWindow;

typedef tgui_u32 TGuiWindowHandle;

TGuiWindow *tgui_window_alloc(TGuiDockerNode *parent, char *name, TGuiWindowFlags flags, struct TGuiAllocatedWindow *list);

TGuiWindow *tgui_window_get_from_handle(TGuiWindowHandle window);

TGuiWindowHandle tgui_create_root_window(char *name, TGuiWindowFlags flags);

TGuiWindowHandle tgui_split_window(TGuiWindowHandle window, TGuiSplitDirection dir, char *name, TGuiWindowFlags flags);

void tgui_window_set_transparent(TGuiWindowHandle window, tgui_b32 state);

tgui_b32 tgui_window_flag_is_set(TGuiWindow *window, TGuiWindowFlags flags);

void tgui_window_flag_set(TGuiWindow *window, TGuiWindowFlags flag);

void tgui_window_flag_clear(TGuiWindow *window, TGuiWindowFlags flag);

typedef void (*TGuiWidgetInternalFunc) (struct TGuiWidget *widget, TGuiPainter *painter);

typedef struct TGuiWidget {
    tgui_u64 id;
    tgui_s32 x, y, w, h;    

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
    
    TGuiArena arena;
    TGuiVirtualMap registry;
    
    tgui_u32 window_id_generator;

    TGuiAllocatedWindow *allocated_windows;
    TGuiAllocatedWindow *free_windows;

    tgui_u64 hot;
    tgui_u64 active;
    
    tgui_f32 dt;
    
    TGuiWidget *first_free_widget;
    
    tgui_u64 active_id;
    TGuiWindow *active_window;
    
    TGuiRenderState render_state;
    
    void *default_texture;
    void *default_program;
    TGuiTextureAtlas *default_texture_atlas;

} TGui;

void tgui_initialize(tgui_s32 w, tgui_s32 h, TGuiGfxBackend *gfx);

void tgui_terminate(void);

void tgui_begin(tgui_f32 dt);

void tgui_end(void);

void tgui_draw_buffers(void);

void tgui_try_to_load_data_file(void);

void tgui_free_allocated_windows_list(struct TGuiAllocatedWindow *list);

TGuiInput *tgui_get_input(void);

TGuiCursor tgui_get_cursor_state(void);

#define tgui_safe_dereference(ptr, type) (((ptr) == NULL) ? (type){0} : *((type *)ptr))

/* --------------------------- */
/*       TGui Framebuffer      */
/* --------------------------- */

void tgui_texture(TGuiWindowHandle window, void *texture);

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

#define tgui_dropdown_menu(window, x, y, options, options_size, options_ptr) _tgui_dropdown_menu((window), (x), (y), (options), (options_size), (options_ptr), TGUI_ID)

typedef struct TGuiButton {
    char *label;
    tgui_b32 result;
} TGuiButton;

tgui_b32 _tgui_button(TGuiWindowHandle window, char *label, tgui_s32 x, tgui_s32 y, char *tgui_id);

void _tgui_button_internal(TGuiWidget *widget, TGuiPainter *painter);

#define TGUI_TEXT_INPUT_MAX_CHARACTERS 124 
typedef struct TGuiTextInput {
    tgui_u8 buffer[TGUI_TEXT_INPUT_MAX_CHARACTERS];
    tgui_u32 used;
    tgui_u32 cursor;
    
    tgui_u32 offset;
    
    tgui_b32 initilize;
    
    tgui_b32 selection;
    tgui_u32 selection_start;
    tgui_u32 selection_end;
    
    tgui_f32 cursor_inactive_acumulator;
    tgui_f32 cursor_inactive_target;
    tgui_f32 cursor_blink_acumulator;
    tgui_f32 cursor_blink_target;
    tgui_b32 blink_cursor;
    tgui_b32 draw_cursor;

} TGuiTextInput;

TGuiTextInput *_tgui_text_input(TGuiWindowHandle window, tgui_s32 x, tgui_s32 y, char *tgui_id);

void _tgui_text_input_internal(TGuiWidget *widget, TGuiPainter *painter);

typedef struct TGuiColorPicker {
    
    TGuiBitmap radiant;
    TGuiBitmap mini_radiant;
    
    tgui_u32 saved_radiant_color;

    tgui_f32 hue;
    tgui_f32 saturation;
    tgui_f32 value;
    
    tgui_b32 hue_cursor_active;
    tgui_b32 sv_cursor_active;

    tgui_u32 *color_ptr;

    tgui_b32 initialize;

} TGuiColorPicker;

void tgui_tgui_u32_color_to_hsv_color(tgui_u32 color, tgui_f32 *h, tgui_f32 *s, tgui_f32 *v);

void _tgui_color_picker(TGuiWindowHandle window, tgui_s32 x, tgui_s32 y, tgui_s32 w, tgui_s32 h, tgui_u32 *color, char *tgui_id);

void _tgui_color_picker_internal(TGuiWidget *widget, TGuiPainter *painter);

typedef struct TGuiTreeViewNode {
        
    char *label;
    tgui_u32 label_depth;
    
    TGuiRectangle dim;
    
    tgui_u32 state_index;
    tgui_b32 visible;
    
    tgui_b32 selected;
    tgui_u32 selected_state_index;
    
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

TGuiArray(tgui_b32, TGuiB32Array);

typedef struct TGuiTreeView {
    
    TGuiTreeViewNode *root;
    TGuiTreeViewNode *active_root_node;
    tgui_s32 active_depth;
    
    TGuiTreeViewNode *first_free_node;
    
    TGuiRectangle dim;
    
    /* TODO: Make this two arrays dynamic or use hash table or link list */
#if 0
    tgui_b32 root_node_state[TGUI_TREEVIEW_MAX_STATE_SIZE];
    tgui_u32 root_node_state_head; 

    tgui_b32 selected_node_data[TGUI_TREEVIEW_MAX_STATE_SIZE];
    tgui_u32 selected_node_data_head;
#endif
    
    TGuiB32Array root_node_state;
    tgui_u32 root_node_state_head; 
    TGuiB32Array selected_node_data;
    tgui_u32 selected_node_data_head;
   
    tgui_u32 rect_w;
    tgui_u32 padding;

    tgui_b32 initiliaze;

    tgui_s32 selection_index;
    void *selection_data;

} TGuiTreeView;

void _tgui_tree_view_begin(TGuiWindowHandle window, char *tgui_id);

void _tgui_tree_view_end(void **selected_data);

void _tgui_tree_view_root_node_begin(char *label, void *user_data);

void _tgui_tree_view_root_node_end(void);

void _tgui_tree_view_node(char *label, void *user_data);

void _tgui_tree_view_internal(TGuiWidget *widget, TGuiPainter *painter);

#define TGUI_DROPDOWN_MENU_DELFAUT_W 140
#define TGUI_DROPDOWN_MENU_DELFAUT_H 26 

typedef struct TGuiDropDownMenu {
    
    char **options;
    tgui_u32 options_size;
    tgui_u32 selected_option;
    
    tgui_b32 click_was_in_scrollbar;

    tgui_b32 initialize;

} TGuiDropDownMenu;

void _tgui_dropdown_menu(TGuiWindowHandle window, tgui_s32 x, tgui_s32 y, char **options, tgui_u32 options_size, tgui_s32 *selected_option_index, char *tgui_id);

void _tgui_dropdown_menu_internal(TGuiWidget *widget, TGuiPainter *painter);

/* ---------------------- */
/*       TGui Font        */
/* ---------------------- */

typedef struct TGuiGlyph {
    TGuiBitmap bitmap;
    tgui_s32 codepoint;

    tgui_s32 top_bearing;
    tgui_s32 left_bearing;
    tgui_s32 adv_width;

} TGuiGlyph;

typedef struct TGuiFont {
    TGuiGlyph *glyphs;
    
    tgui_u32 glyph_count;
    tgui_u32 glyph_rage_start;
    tgui_u32 glyph_rage_end;

    tgui_s32 ascent;
    tgui_s32 descent;
    tgui_s32 line_gap;

    tgui_u32 max_glyph_height;
    tgui_u32 max_glyph_width;
    
    struct TGuiOsFont *font;
} TGuiFont;

void tgui_font_initilize(TGuiArena *arena);

void tgui_font_terminate(void);

TGuiGlyph *tgui_font_get_codepoint_glyph(tgui_u32 codepoint);

TGuiRectangle tgui_get_size_text_dim(tgui_s32 x, tgui_s32 y, char *text, tgui_u32 size);

TGuiRectangle tgui_get_text_dim(tgui_s32 x, tgui_s32 y, char *text);

void tgui_font_draw_text(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, char *text, tgui_u32 size, tgui_u32 color);

#endif /* _TGUI_H_ */
