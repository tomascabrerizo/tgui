#ifndef _PAINTER_H_
#define _PAINTER_H_

#include "common.h"
#include "geometry.h"
#include "memory.h"

typedef struct Point {
    s32 x, y;
} Point;

typedef struct Bitmap {
    u32 *pixels;
    u32 width, height;
} Bitmap;

typedef struct Painter {
    u32 *pixels;
    Rectangle dim;
    Rectangle clip;
} Painter;

typedef struct PainterGlyph {
    Bitmap bitmap;
    s32 codepoint;

    s32 top_bearing;
    s32 left_bearing;
    s32 adv_width;

} PainterGlyph;

typedef struct PainterFont {
    PainterGlyph *glyphs;
    
    u32 glyph_count;
    u32 glyph_rage_start;
    u32 glyph_rage_end;

    s32 ascent;
    s32 descent;
    s32 line_gap;
    
    struct OsFont *font;

} PainterFont;

void painter_initialize(Arena *arena);

void painter_terminate(void);

void painter_start(Painter *painter, u32 *pixels, Rectangle dim, Rectangle *clip);

void painter_clear(Painter *painter, u32 color);

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color);

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color);

void painter_draw_rectangle_outline(Painter *painter, Rectangle rectangle, u32 color);

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color);

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color);

void painter_draw_line(Painter *painter, s32 x0, s32 y0, s32 x1, s32 y1, u32 color);

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, Bitmap *bitmap, u32 tint);

void painter_draw_text(Painter *painter, s32 x, s32 y, char *text, u32 color);

Rectangle painter_get_text_dim(Painter *painter, s32 x, s32 y, char *text);

#endif /* _PAINTER_H_ */
