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

    tgui_u32 *pixels;

    struct TGuiRenderBuffer *render_buffer;

} TGuiPainter;

struct TGuiBitmap;

void tgui_painter_start(TGuiPainter *painter, TGuiPainterType type, TGuiRectangle dim, TGuiRectangle *clip, tgui_u32 *pixels, struct TGuiRenderBuffer *render_buffer);

void tgui_painter_clear(TGuiPainter *painter, tgui_u32 color);

void tgui_painter_draw_rect(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, tgui_s32 w, tgui_s32 h, tgui_u32 color);

void tgui_painter_draw_rectangle(TGuiPainter *painter, TGuiRectangle rectangle, tgui_u32 color);

void tgui_painter_draw_rectangle_outline(TGuiPainter *painter, TGuiRectangle rectangle, tgui_u32 color);

void tgui_painter_draw_vline(TGuiPainter *painter, tgui_s32 x, tgui_s32 y0, tgui_s32 y1, tgui_u32 color);

void tgui_painter_draw_hline(TGuiPainter *painter, tgui_s32 y, tgui_s32 x0, tgui_s32 x1, tgui_u32 color);

void tgui_painter_draw_bitmap(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, struct TGuiBitmap *bitmap, tgui_u32 tint);

void tgui_painter_draw_bitmap_no_alpha(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, struct TGuiBitmap *bitmap);

void tgui_painter_draw_render_buffer_texture(TGuiPainter *painter, TGuiRectangle dim);

#endif /* _TGUI_PAINTER_H_ */
