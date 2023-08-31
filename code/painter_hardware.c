#include "common.h"
#include "painter.h"

void painter_start(Painter *painter, u32 *pixels, Rectangle dim, Rectangle *clip) {
    UNUSED(painter); UNUSED(pixels); UNUSED(dim); UNUSED(clip);
}

void painter_clear(Painter *painter, u32 color) {
    UNUSED(painter); UNUSED(color);
}

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color) {
    UNUSED(painter); UNUSED(x); UNUSED(y); UNUSED(w); UNUSED(h); UNUSED(color);
}

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color) {
    UNUSED(painter); UNUSED(rectangle); UNUSED(color);
}

void painter_draw_rectangle_outline(Painter *painter, Rectangle rectangle, u32 color) {
    UNUSED(painter); UNUSED(rectangle); UNUSED(color);
}

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color) {
    UNUSED(painter); UNUSED(x); UNUSED(y0); UNUSED(y1); UNUSED(color);
}

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color) {
    UNUSED(painter); UNUSED(y); UNUSED(x0); UNUSED(x1); UNUSED(color);
}

void painter_draw_line(Painter *painter, s32 x0, s32 y0, s32 x1, s32 y1, u32 color) {
    UNUSED(painter); UNUSED(y0); UNUSED(y1); UNUSED(x0); UNUSED(x1); UNUSED(color);
}

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap, u32 tint) {
    UNUSED(painter); UNUSED(x); UNUSED(y); UNUSED(bitmap); UNUSED(tint);
}

void painter_draw_bitmap_no_alpha(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap) {
    UNUSED(painter); UNUSED(x); UNUSED(y); UNUSED(bitmap);
}

