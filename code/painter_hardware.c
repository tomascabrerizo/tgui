#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"

void painter_start(Painter *painter, Rectangle dim, Rectangle *clip, u32 *pixels, TGuiVertexArray *vertex_array, TGuiU32Array *index_array) {
    UNUSED(pixels);

    painter->dim = dim;
    if(clip) {
        painter->clip = *clip;
    } else {
        painter->clip = dim;
    }
    
    ASSERT(vertex_array);
    ASSERT(index_array);

    painter->vertex_buffer = vertex_array;
    painter->index_buffer = index_array;

    tgui_array_clear(painter->vertex_buffer);
    tgui_array_clear(painter->index_buffer);
}

void painter_clear(Painter *painter, u32 color) {
    painter_draw_rectangle(painter, painter->clip, color);
}

static void setup_vertex(TGuiVertex *vertex, s32 x, s32 y, f32 u, f32 v, u32 color) {
    
    vertex->x = (f32)x;
    vertex->y = (f32)y;
    vertex->u = u;
    vertex->v = v;
    
    f32 inv_255 = 1.0f / 255.0f;

    vertex->r = ((color >> 16) & 0xff) * inv_255;
    vertex->g = ((color >>  8) & 0xff) * inv_255;
    vertex->b = ((color >>  0) & 0xff) * inv_255;
}

static inline void clip_rectangle(Rectangle *rect, Rectangle clip, s32 *offset_x, s32 *offset_y) {
    
    if(rect->max_x > clip.max_x) rect->max_x = clip.max_x;
    if(rect->max_y > clip.max_y) rect->max_y = clip.max_y;
    
    s32 ox = 0;
    s32 oy = 0;

    if(rect->min_x < clip.min_x) {
        ox = clip.min_x - rect->min_x;
        rect->min_x = clip.min_x;
    }
    
    if(rect->min_y < clip.min_y) {
        oy = clip.min_y - rect->min_y;
        rect->min_y = clip.min_y;
    }

    if(offset_x) *offset_x = ox;
    if(offset_y) *offset_y = oy;
}

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color) {
   
    clip_rectangle(&rectangle, painter->clip, 0, 0);
    
    if(rect_invalid(rectangle)) return;

    rectangle.max_x += 1;
    rectangle.max_y += 1;
    
    u32 start_vertex_index = tgui_array_size(painter->vertex_buffer);

    TGuiVertex *vertex0 = tgui_array_push(painter->vertex_buffer);
    TGuiVertex *vertex1 = tgui_array_push(painter->vertex_buffer);
    TGuiVertex *vertex2 = tgui_array_push(painter->vertex_buffer);
    TGuiVertex *vertex3 = tgui_array_push(painter->vertex_buffer);
    
    setup_vertex(vertex0, rectangle.min_x, rectangle.min_y, 0.0f, 0.0f, color);
    setup_vertex(vertex1, rectangle.min_x, rectangle.max_y, 0.0f, 0.0f, color);
    setup_vertex(vertex2, rectangle.max_x, rectangle.max_y, 0.0f, 0.0f, color);
    setup_vertex(vertex3, rectangle.max_x, rectangle.min_y, 0.0f, 0.0f, color);
    
    u32 *index0 = tgui_array_push(painter->index_buffer);
    u32 *index1 = tgui_array_push(painter->index_buffer);
    u32 *index2 = tgui_array_push(painter->index_buffer);

    *index0 = start_vertex_index + 0;
    *index1 = start_vertex_index + 1;
    *index2 = start_vertex_index + 2;

    u32 *index3 = tgui_array_push(painter->index_buffer);
    u32 *index4 = tgui_array_push(painter->index_buffer);
    u32 *index5 = tgui_array_push(painter->index_buffer);

    *index3 = start_vertex_index + 2;
    *index4 = start_vertex_index + 3;
    *index5 = start_vertex_index + 0;

}

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color) {
    Rectangle rectangle = rect_from_wh(x, y, w, h);
    painter_draw_rectangle(painter, rectangle, color);
}

void painter_draw_rectangle_outline(Painter *painter, Rectangle rectangle, u32 color) {
    Rectangle l = rectangle;
    l.max_x = l.min_x;
    Rectangle r = rectangle;
    r.min_x = r.max_x;
    painter_draw_rectangle(painter, l, color);
    painter_draw_rectangle(painter, r, color);
    
    Rectangle b = rectangle;
    b.min_y = b.max_y;
    Rectangle t = rectangle;
    t.max_y = t.min_y;
    painter_draw_rectangle(painter, t, color);
    painter_draw_rectangle(painter, b, color);
}

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap, u32 tint) {
    UNUSED(painter); UNUSED(x); UNUSED(y); UNUSED(bitmap); UNUSED(tint);
}

void painter_draw_bitmap_no_alpha(Painter *painter, s32 x, s32 y, struct TGuiBitmap *bitmap) {
    painter_draw_bitmap(painter, x, y, bitmap, 0xffffff);
}

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color) {
    Rectangle vline;
    vline.min_x = x;
    vline.max_x = x;
    vline.min_y = y0;
    vline.max_y = y1;
    painter_draw_rectangle(painter, vline, color);
}

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color) {
    Rectangle hline;
    hline.min_y = y;
    hline.max_y = y;
    hline.min_x = x0;
    hline.max_x = x1;
    painter_draw_rectangle(painter, hline, color);
}

void painter_draw_line(Painter *painter, s32 x0, s32 y0, s32 x1, s32 y1, u32 color) {
    UNUSED(painter); UNUSED(y0); UNUSED(y1); UNUSED(x0); UNUSED(x1); UNUSED(color);
}

