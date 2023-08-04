#ifndef _OS_H_
#define _OS_H_

#include "common.h"

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

struct OsWindow *os_window_create(struct Arena *arena, char *title, u32 x, u32 y, u32 w, u32 h);

void os_window_destroy(struct OsWindow *window);

void os_window_poll_events(struct OsWindow *window);

s32 os_window_width(struct OsWindow *window);

s32 os_window_height(struct OsWindow *window);

b32 os_window_should_close(struct OsWindow *window);

b32 os_window_resize(struct OsWindow *window);

void os_window_get_mouse_position(struct OsWindow *window, s32 *x, s32 *y);

void os_window_get_mouse_lbutton_state(struct OsWindow *window, b32 *is_down);


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


#endif /* _OS_H_ */

