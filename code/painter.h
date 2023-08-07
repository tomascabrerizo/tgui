#ifndef _PAINTER_H_
#define _PAINTER_H_

#include "common.h"
#include "geometry.h"

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

void painter_initialize(Painter *painter, u32 *pixels, Rectangle dim, Rectangle *clip);

void painter_clear(Painter *painter, u32 color);

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color);

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color);

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color);

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color);

void painter_draw_line(Painter *painter, s32 x0, s32 y0, s32 x1, s32 y1, u32 color);

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, Bitmap *bitmap);

#endif /* _PAINTER_H_ */
