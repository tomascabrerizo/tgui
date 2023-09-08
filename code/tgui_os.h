#ifndef _TGUI_OS_H_
#define _TGUI_OS_H_

#include "tgui_common.h"

void tgui_os_initialize(void);

void tgui_os_terminate(void);


/* -------------------
        Memory
   ------------------- */

tgui_u64 tgui_os_get_page_size(void);

void *tgui_os_virtual_reserve(tgui_u64 size);

void tgui_os_virtual_commit(void *ptr, tgui_u64 size);

void tgui_os_virtual_decommit(void *ptr, tgui_u64 size);

void tgui_os_virtual_release(void *ptr, tgui_u64 size);


/* -------------------------
       File Manager 
   ------------------------- */

typedef struct TGuiOsFile {
    void *data;
    tgui_u64 size;
} TGuiOsFile;

TGuiOsFile *tgui_os_file_read_entire(const char *path);

void tgui_os_file_free(TGuiOsFile *file);


/* -------------------------
        Font Rasterizer 
   ------------------------- */

struct TGuiArena;
struct TGuiOsFont;

struct TGuiOsFont *tgui_os_font_create(struct TGuiArena *arena, const char *path, tgui_u32 size);

void tgui_os_font_destroy(struct TGuiOsFont *font);

void tgui_os_font_rasterize_glyph(struct TGuiOsFont *font, tgui_u32 codepoint, void **buffer, tgui_s32 *w, tgui_s32 *h, tgui_s32 *bpp);

tgui_s32 tgui_os_font_get_kerning_between(struct TGuiOsFont *font, tgui_u32 codepoint0, tgui_u32 codepoint1);

void tgui_os_font_get_vmetrics(struct TGuiOsFont *font, tgui_s32 *ascent, tgui_s32 *descent, tgui_s32 *line_gap); 

void tgui_os_font_get_glyph_metrics(struct TGuiOsFont *font, tgui_u32 codepoint, tgui_s32 *adv_width, tgui_s32 *left_bearing, tgui_s32 *top_bearing);

#endif /* _TGUI_OS_H_ */

