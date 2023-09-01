#ifndef _PAINTER_H_
#define _PAINTER_H_

#include "common.h"
#include "geometry.h"
#include "memory.h"

typedef struct TGuiVertex {
    float x, y;
    float u, v;
    float r, g, b;
} TGuiVertex;

TGuiArray(TGuiVertex, TGuiVertexArray);
TGuiArray(u32, TGuiU32Array);

typedef enum PainterType {
    PAINTER_TYPE_SOFTWARE,
    PAINTER_TYPE_HARDWARE,
} PainterType;

typedef struct Painter {

    PainterType type;

    Rectangle dim;
    Rectangle clip;

    TGuiVertexArray *vertex_buffer;
    TGuiU32Array    *index_buffer;
    u32 *pixels;
} Painter;

struct TGuiBitmap;

void painter_start(Painter *painter, PainterType type, Rectangle dim, Rectangle *clip, u32 *pixels, TGuiVertexArray *vertex_array, TGuiU32Array *index_array);

void painter_clear(Painter *painter, u32 color);

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color);

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color);

void painter_draw_rectangle_outline(Painter *painter, Rectangle rectangle, u32 color);

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color);

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color);

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap, u32 tint);

void painter_draw_bitmap_no_alpha(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap);

#endif /* _PAINTER_H_ */
