#include "tgui_os.h"

#include "tgui_memory.h"
#include "stb_truetype.h"

#include <stdlib.h>
#include <sys/mman.h>

static inline void tgui_os_error(void) {
    printf("OS Error: %s\n", strerror(errno));
    ASSERT(!"Fatal error!");
}

static u64 g_os_page_size = 0;

void tgui_os_initialize(void) {

    long result = sysconf(_SC_PAGESIZE); 
    if(result == -1) {
        tgui_os_error();
    }
    g_os_page_size = (u64)result; 
}

void tgui_os_terminate(void) {

}

/* -------------------------
       File Manager 
   ------------------------- */

TGuiOsFile *tgui_os_file_read_entire(const char *path) {
    TGuiOsFile *result = malloc(sizeof(TGuiOsFile));

    FILE *file = fopen((char *)path, "rb");
    if(!file) {
        printf("Cannot load file: %s\n", path);
        free(result);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    u64 file_size = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    result = (TGuiOsFile *)malloc(sizeof(TGuiOsFile) + file_size+1);
    
    result->data = result + 1;
    result->size = file_size;

    ASSERT(((u64)result->data % 8) == 0);
    fread(result->data, file_size+1, 1, file);
    ((u8 *)result->data)[file_size] = '\0';
    
    fclose(file);

    return result;
}

void tgui_os_file_free(TGuiOsFile *file) {
    free(file);
}

/* -------------------------
        Font Rasterizer 
   ------------------------- */

typedef struct TGuiOsFont {
    TGuiOsFile *file;
    stbtt_fontinfo info;
    u32 size;
    f32 size_ratio;
} TGuiOsFont;

struct TGuiOsFont *tgui_os_font_create(struct TGuiArena *arena, const char *path, u32 size) {
    TGuiOsFont *font = tgui_arena_push_struct(arena, TGuiOsFont, 8);
    font->size = size;
    font->file = tgui_os_file_read_entire(path);
    stbtt_InitFont(&font->info, font->file->data, stbtt_GetFontOffsetForIndex(font->file->data,0));
    font->size_ratio = stbtt_ScaleForPixelHeight(&font->info, size);
    return font;
}

void tgui_os_font_destroy(struct TGuiOsFont *font) {
    tgui_os_file_free(font->file);
}

void tgui_os_font_rasterize_glyph(struct TGuiOsFont *font, u32 codepoint, void **buffer, s32 *w, s32 *h, s32 *bpp) {
   *buffer = stbtt_GetCodepointBitmap(&font->info, 0, font->size_ratio, codepoint, w, h, 0,0);
   *bpp = 1;
}

s32 tgui_os_font_get_kerning_between(struct TGuiOsFont *font, u32 codepoint0, u32 codepoint1) {
    return stbtt_GetCodepointKernAdvance(&font->info, codepoint0, codepoint1) * font->size_ratio;
}

void tgui_os_font_get_vmetrics(struct TGuiOsFont *font, s32 *ascent, s32 *descent, s32 *line_gap) { 
    stbtt_GetFontVMetrics(&font->info, ascent, descent, line_gap);
    *ascent   *= font->size_ratio;
    *descent  *= font->size_ratio;
    *line_gap *= font->size_ratio;
}

void tgui_os_font_get_glyph_metrics(struct TGuiOsFont *font, u32 codepoint, s32 *adv_width, s32 *left_bearing, s32 *top_bearing) {
    stbtt_GetCodepointHMetrics(&font->info, codepoint, adv_width, left_bearing);
    *adv_width    *= font->size_ratio;
    *left_bearing *= font->size_ratio;
    s32 x0, y0, x1, y1;
    stbtt_GetCodepointBox(&font->info, codepoint, &x0, &y0, &x1, &y1);
    y1 *= font->size_ratio;
    *top_bearing = y1;
}

/* -------------------
        Memory
   ------------------- */

u64 tgui_os_get_page_size(void) {
    ASSERT(g_os_page_size > 0);
    return g_os_page_size;
}

void *tgui_os_virtual_reserve(u64 size) {

    ASSERT(IS_POWER_OF_TWO(tgui_os_get_page_size()));
    ASSERT((size & (tgui_os_get_page_size() - 1)) == 0);

    int prot = PROT_NONE;
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    off_t offset = 0;
    void *address = mmap(NULL, (size_t)size, prot, flags, -1, offset);
    
    if(address == MAP_FAILED) {
        tgui_os_error();
    }

    return address;
}

void tgui_os_virtual_commit(void *ptr, u64 size) {

    ASSERT(IS_POWER_OF_TWO(tgui_os_get_page_size()));
    ASSERT(((u64)ptr & (tgui_os_get_page_size() - 1)) == 0);
    ASSERT((size & (tgui_os_get_page_size() - 1)) == 0);

    int prot = PROT_READ | PROT_WRITE;
    int result = 0;
    
    result = madvise(ptr, size, MADV_WILLNEED);
    if(result == -1) {
        tgui_os_error();
    }
    
    result = mprotect(ptr, (size_t)size, prot);
    if(result == -1) {
        tgui_os_error();
    }
}

void tgui_os_virtual_decommit(void *ptr, u64 size) {
    
    ASSERT(IS_POWER_OF_TWO(tgui_os_get_page_size()));
    ASSERT(((u64)ptr & (tgui_os_get_page_size() - 1)) == 0);
    ASSERT((size & (tgui_os_get_page_size() - 1)) == 0);

    int prot = PROT_NONE;
    int result = 0;
    
    result = madvise(ptr, size, MADV_DONTNEED);
    if(result == -1) {
        tgui_os_error();
    }

    result = mprotect(ptr, (size_t)size, prot);
    if(result == -1) {
        tgui_os_error();
    }
}

void tgui_os_virtual_release(void *ptr, u64 size) {
    
    ASSERT(IS_POWER_OF_TWO(tgui_os_get_page_size()));
    ASSERT(((u64)ptr & (tgui_os_get_page_size() - 1)) == 0);
    ASSERT((size & (tgui_os_get_page_size() - 1)) == 0);
    
    int result = munmap(ptr, size);
    if(result == -1) {
        tgui_os_error();
    }
}

