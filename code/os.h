#ifndef _OS_H_
#define _OS_H_

#include "common.h"
#include "memory.h"
#include "tgui.h"

void os_initialize(void);

void os_terminate(void);

/* -------------
       Time 
   ------------- */

u64 os_get_ticks(void);

void os_sleep(u64 milliseconds);

/* ---------------------------
        Windows and Inputs 
   --------------------------- */

struct Arena;
struct OsWindow;
struct TGuiKeyboard;

struct OsWindow *os_window_create(struct Arena *arena, char *title, u32 x, u32 y, u32 w, u32 h);

void os_window_destroy(struct OsWindow *window);

void os_window_poll_events(struct OsWindow *window);

s32 os_window_width(struct OsWindow *window);

s32 os_window_height(struct OsWindow *window);

b32 os_window_should_close(struct OsWindow *window);

b32 os_window_resize(struct OsWindow *window);

void os_window_get_mouse_position(struct OsWindow *window, s32 *x, s32 *y);

void os_window_get_mouse_lbutton_state(struct OsWindow *window, b32 *is_down);

void os_window_get_text_input(struct OsWindow *window, u8 *buffer, u32 *size, u32 max_size);

void os_window_get_keyboard_input(struct OsWindow *window, struct TGuiKeyboard *keyborad);


/* ---------------------
        Backbuffer 
   --------------------- */

struct OsBackbuffer;

struct OsBackbuffer *os_backbuffer_create(struct Arena *arena, struct OsWindow *window, void *data, u32 w, u32 h, u32 bytes_per_pixel);

void os_backbuffer_destroy(struct OsBackbuffer *backbuffer);

void os_backbuffer_swap(struct OsWindow *window, struct OsBackbuffer *backbuffer);


/* -------------------
        Memory
   ------------------- */

u64 os_get_page_size(void);

void *os_virtual_reserve(u64 size);

void os_virtual_commit(void *ptr, u64 size);

void os_virtual_decommit(void *ptr, u64 size);

void os_virtual_release(void *ptr, u64 size);


/* -------------------
        Cursor 
   ------------------- */

typedef enum OsCursor {
    OS_CURSOR_ARROW,
    OS_CURSOR_HAND,
    OS_CURSOR_V_ARROW,
    OS_CURSOR_H_ARROW,
} OsCursor;

void os_set_cursor(struct OsWindow *window, OsCursor cursor);


/* -------------------------
        Font Rasterizer 
   ------------------------- */

struct OsFont;

struct OsFont *os_font_create(struct Arena *arena, const char *path, u32 size);

void os_font_destroy(struct OsFont *font);

void os_font_rasterize_glyph(struct OsFont *font, u32 codepoint, void **buffer, s32 *w, s32 *h, s32 *bpp);

s32 os_font_get_kerning_between(struct OsFont *font, u32 codepoint0, u32 codepoint1);

void os_font_get_vmetrics(struct OsFont *font, s32 *ascent, s32 *descent, s32 *line_gap); 

void os_font_get_glyph_metrics(struct OsFont *font, u32 codepoint, s32 *adv_width, s32 *left_bearing, s32 *top_bearing);

#endif /* _OS_H_ */

