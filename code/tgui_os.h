#ifndef _TGUI_OS_H_
#define _TGUI_OS_H_

#include "common.h"

void tgui_os_initialize(void);

void tgui_os_terminate(void);


/* -------------------
        Memory
   ------------------- */

u64 tgui_os_get_page_size(void);

void *tgui_os_virtual_reserve(u64 size);

void tgui_os_virtual_commit(void *ptr, u64 size);

void tgui_os_virtual_decommit(void *ptr, u64 size);

void tgui_os_virtual_release(void *ptr, u64 size);


/* -------------------------
       File Manager 
   ------------------------- */

typedef struct TGuiOsFile {
    void *data;
    u64 size;
} TGuiOsFile;

TGuiOsFile *tgui_os_file_read_entire(const char *path);

void tgui_os_file_free(TGuiOsFile *file);


/* -------------------------
        Font Rasterizer 
   ------------------------- */

struct TGuiArena;
struct TGuiOsFont;

struct TGuiOsFont *tgui_os_font_create(struct TGuiArena *arena, const char *path, u32 size);

void tgui_os_font_destroy(struct TGuiOsFont *font);

void tgui_os_font_rasterize_glyph(struct TGuiOsFont *font, u32 codepoint, void **buffer, s32 *w, s32 *h, s32 *bpp);

s32 tgui_os_font_get_kerning_between(struct TGuiOsFont *font, u32 codepoint0, u32 codepoint1);

void tgui_os_font_get_vmetrics(struct TGuiOsFont *font, s32 *ascent, s32 *descent, s32 *line_gap); 

void tgui_os_font_get_glyph_metrics(struct TGuiOsFont *font, u32 codepoint, s32 *adv_width, s32 *left_bearing, s32 *top_bearing);

#endif /* _TGUI_OS_H_ */

