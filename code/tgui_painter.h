#ifndef _TGUI_PAINTER_H_
#define _TGUI_PAINTER_H_

#include "tgui_geometry.h"

typedef enum TGuiPainterType {
    TGUI_PAINTER_TYPE_SOFTWARE,
    TGUI_PAINTER_TYPE_HARDWARE,
} TGuiPainterType;

typedef struct TGuiPainter {

    TGuiPainterType type;

    TGuiRectangle dim;
    TGuiRectangle clip;

    u32 *pixels;

    struct TGuiRenderBuffer *render_buffer;

} TGuiPainter;

struct TGuiBitmap;

void tgui_painter_start(TGuiPainter *painter, TGuiPainterType type, TGuiRectangle dim, TGuiRectangle *clip, u32 *pixels, struct TGuiRenderBuffer *render_buffer);

void tgui_painter_clear(TGuiPainter *painter, u32 color);

void tgui_painter_draw_rect(TGuiPainter *painter, s32 x, s32 y, s32 w, s32 h, u32 color);

void tgui_painter_draw_rectangle(TGuiPainter *painter, TGuiRectangle rectangle, u32 color);

void tgui_painter_draw_rectangle_outline(TGuiPainter *painter, TGuiRectangle rectangle, u32 color);

void tgui_painter_draw_vline(TGuiPainter *painter, s32 x, s32 y0, s32 y1, u32 color);

void tgui_painter_draw_hline(TGuiPainter *painter, s32 y, s32 x0, s32 x1, u32 color);

void tgui_painter_draw_bitmap(TGuiPainter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap, u32 tint);

void tgui_painter_draw_bitmap_no_alpha(TGuiPainter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap);

void tgui_painter_draw_render_buffer_texture(TGuiPainter *painter, TGuiRectangle dim);

#endif /* _TGUI_PAINTER_H_ */
