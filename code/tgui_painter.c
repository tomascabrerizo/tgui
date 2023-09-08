#include "tgui_painter.h"

#include "tgui_gfx.h"

static inline void clip_rectangle(TGuiRectangle *rect, TGuiRectangle clip, tgui_s32 *offset_x, tgui_s32 *offset_y) {
    
    if(rect->max_x > clip.max_x) rect->max_x = clip.max_x;
    if(rect->max_y > clip.max_y) rect->max_y = clip.max_y;
    
    tgui_s32 ox = 0;
    tgui_s32 oy = 0;

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

static void setup_vertex(TGuiVertex *vertex, tgui_s32 x, tgui_s32 y, tgui_f32 u, tgui_f32 v, tgui_u32 color) {
    
    vertex->x = (tgui_f32)x;
    vertex->y = (tgui_f32)y;
    vertex->u = u;
    vertex->v = v;
    
    tgui_f32 inv_255 = 1.0f / 255.0f;

    vertex->r = ((color >> 16) & 0xff) * inv_255;
    vertex->g = ((color >>  8) & 0xff) * inv_255;
    vertex->b = ((color >>  0) & 0xff) * inv_255;
}

void tgui_painter_draw_pixel(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, tgui_u32 color) {
    if(x >= painter->clip.min_x && x <= painter->clip.max_x &&
       y >= painter->clip.min_y && y <= painter->clip.max_y) {
        tgui_u32 painter_w = tgui_rect_width(painter->clip);
        tgui_u32 *pixel = painter->pixels + (y * painter_w) + x;
        *pixel = color;
    }
}


void tgui_painter_start(TGuiPainter *painter, TGuiPainterType type, TGuiRectangle dim, TGuiRectangle *clip, tgui_u32 *pixels, TGuiRenderBuffer *render_buffer) {
    painter->type = type;

    painter->pixels = pixels;
    painter->dim = dim;

    if(clip) {
        painter->clip = *clip;
    } else {
        painter->clip = dim;
    }
    
    if(painter->type == TGUI_PAINTER_TYPE_HARDWARE) {
        painter->render_buffer = render_buffer;
    }

}

void tgui_painter_draw_rectangle(TGuiPainter *painter, TGuiRectangle rectangle, tgui_u32 color) {
    
    clip_rectangle(&rectangle, painter->clip, 0, 0);

    switch (painter->type) {
    case TGUI_PAINTER_TYPE_HARDWARE: {
        
        if(tgui_rect_invalid(rectangle)) return;

        TGuiVertexArray *vertex_buffer = &painter->render_buffer->vertex_buffer;
        TGuiU32Array *index_buffer = &painter->render_buffer->index_buffer;

        rectangle.max_x += 1;
        rectangle.max_y += 1;
        
        tgui_u32 start_vertex_index = tgui_array_size(vertex_buffer);

        TGuiVertex *vertex0 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex1 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex2 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex3 = tgui_array_push(vertex_buffer);
        
        setup_vertex(vertex0, rectangle.min_x, rectangle.min_y, 0.0f, 0.0f, color);
        setup_vertex(vertex1, rectangle.min_x, rectangle.max_y, 0.0f, 0.0f, color);
        setup_vertex(vertex2, rectangle.max_x, rectangle.max_y, 0.0f, 0.0f, color);
        setup_vertex(vertex3, rectangle.max_x, rectangle.min_y, 0.0f, 0.0f, color);
        
        tgui_u32 *index0 = tgui_array_push(index_buffer);
        tgui_u32 *index1 = tgui_array_push(index_buffer);
        tgui_u32 *index2 = tgui_array_push(index_buffer);

        *index0 = start_vertex_index + 0;
        *index1 = start_vertex_index + 1;
        *index2 = start_vertex_index + 2;

        tgui_u32 *index3 = tgui_array_push(index_buffer);
        tgui_u32 *index4 = tgui_array_push(index_buffer);
        tgui_u32 *index5 = tgui_array_push(index_buffer);

        *index3 = start_vertex_index + 2;
        *index4 = start_vertex_index + 3;
        *index5 = start_vertex_index + 0;
    
    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        tgui_u32 painter_w = tgui_rect_width(painter->dim);
        tgui_u32 *row = painter->pixels + (rectangle.min_y * painter_w) + rectangle.min_x;

        for(tgui_s32 y = rectangle.min_y; y <= rectangle.max_y; ++y) {
            tgui_u32 *pixel = row;
            for(tgui_s32 x = rectangle.min_x; x <= rectangle.max_x; ++x) {
                *pixel++ = color;
            }
            row += painter_w;
        }

    }break;
    
    }

}

void tgui_painter_clear(TGuiPainter *painter, tgui_u32 color) {
    tgui_painter_draw_rectangle(painter, painter->clip, color);
}

void tgui_painter_draw_rect(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, tgui_s32 w, tgui_s32 h, tgui_u32 color) {
    TGuiRectangle rectangle = tgui_rect_from_wh(x, y, w, h);
    tgui_painter_draw_rectangle(painter, rectangle, color);
}

void tgui_painter_draw_rectangle_outline(TGuiPainter *painter, TGuiRectangle rectangle, tgui_u32 color) {

    switch (painter->type) {
    
    case TGUI_PAINTER_TYPE_HARDWARE: {

        TGuiRectangle l = rectangle;
        l.max_x = l.min_x;
        TGuiRectangle r = rectangle;
        r.min_x = r.max_x;
        tgui_painter_draw_rectangle(painter, l, color);
        tgui_painter_draw_rectangle(painter, r, color);
        
        TGuiRectangle b = rectangle;
        b.min_y = b.max_y;
        TGuiRectangle t = rectangle;
        t.max_y = t.min_y;
        tgui_painter_draw_rectangle(painter, t, color);
        tgui_painter_draw_rectangle(painter, b, color);

    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        tgui_painter_draw_hline(painter, rectangle.min_y, rectangle.min_x, rectangle.max_x, color);
        tgui_painter_draw_hline(painter, rectangle.max_y, rectangle.min_x, rectangle.max_x, color);
        tgui_painter_draw_vline(painter, rectangle.min_x, rectangle.min_y, rectangle.max_y, color);
        tgui_painter_draw_vline(painter, rectangle.max_x, rectangle.min_y, rectangle.max_y, color);

    } break;
    
    }

}

void tgui_painter_draw_bitmap_no_alpha(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, struct TGuiBitmap *bitmap) {

    switch (painter->type) {

    case TGUI_PAINTER_TYPE_HARDWARE: {
        tgui_painter_draw_bitmap(painter, x, y, bitmap, 0xffffff);
    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        TGuiRectangle rect;
        rect.min_x = x;
        rect.min_y = y;
        rect.max_x = x + bitmap->width  - 1;
        rect.max_y = y + bitmap->height - 1;
        
        tgui_s32 offset_x;
        tgui_s32 offset_y;
        clip_rectangle(&rect, painter->clip, &offset_x, &offset_y);

        tgui_u32 painter_w = tgui_rect_width(painter->dim);
        tgui_u32 *row = painter->pixels + (rect.min_y * painter_w) + rect.min_x;
        tgui_u32 *src_row = bitmap->pixels + (offset_y * bitmap->width) + offset_x;

        for(y = rect.min_y; y <= rect.max_y; ++y) {
            tgui_u32 *pixel = row;
            tgui_u32 *src_pixel = src_row;
            for(x = rect.min_x; x <= rect.max_x; ++x) {
                *pixel++ = *src_pixel++;
            }
            row += painter_w;
            src_row += bitmap->width;
        }

    } break;

    }
}

void tgui_painter_draw_bitmap(TGuiPainter *painter, tgui_s32 x, tgui_s32 y, TGuiBitmap *bitmap, tgui_u32 tint) {

    TGuiRectangle rectangle;
    rectangle.min_x = x;
    rectangle.min_y = y;
    rectangle.max_x = x + bitmap->width  - 1;
    rectangle.max_y = y + bitmap->height - 1;

    TGuiRectangle unclip_rectangle = rectangle;
    
    tgui_s32 offset_x;
    tgui_s32 offset_y;
    clip_rectangle(&rectangle, painter->clip, &offset_x, &offset_y);

    switch (painter->type) {

    case TGUI_PAINTER_TYPE_HARDWARE: {
        
        if(tgui_rect_invalid(rectangle)) return;

        TGuiVertexArray *vertex_buffer = &painter->render_buffer->vertex_buffer;
        TGuiU32Array *index_buffer = &painter->render_buffer->index_buffer;

        TGUI_ASSERT(bitmap->texture);

        rectangle.max_x += 1;
        rectangle.max_y += 1;

        unclip_rectangle.max_x += 1;
        unclip_rectangle.max_y += 1;

        tgui_u32 start_vertex_index = tgui_array_size(vertex_buffer);

        TGuiVertex *vertex0 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex1 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex2 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex3 = tgui_array_push(vertex_buffer);
        
        tgui_u32 color = tint;

        tgui_u32 min_offset_x = offset_x;
        tgui_u32 min_offset_y = offset_y;
        tgui_u32 max_offset_x = unclip_rectangle.max_x - rectangle.max_x;
        tgui_u32 max_offset_y = unclip_rectangle.max_y - rectangle.max_y;

        TGuiTexture *texture = bitmap->texture;
        TGuiRectangle texture_rectangle = texture->dim;
        texture_rectangle.max_x += 1;
        texture_rectangle.max_y += 1;
        
        TGuiTextureAtlas *texture_atlas = painter->render_buffer->texture_atlas;
        TGUI_ASSERT(texture_atlas);

        tgui_u32 texture_atlas_w = texture_atlas->bitmap.width;
        tgui_u32 texture_atlas_h = texture_atlas->bitmap.height;

        tgui_f32 min_u = (tgui_f32)(texture_rectangle.min_x + min_offset_x) / (tgui_f32)texture_atlas_w; 
        tgui_f32 min_v = (tgui_f32)(texture_rectangle.min_y + min_offset_y) / (tgui_f32)texture_atlas_h;
        tgui_f32 max_u = (tgui_f32)(texture_rectangle.max_x - max_offset_x) / (tgui_f32)texture_atlas_w; 
        tgui_f32 max_v = (tgui_f32)(texture_rectangle.max_y - max_offset_y) / (tgui_f32)texture_atlas_h;

        setup_vertex(vertex0, rectangle.min_x, rectangle.min_y, min_u, min_v, color);
        setup_vertex(vertex1, rectangle.min_x, rectangle.max_y, min_u, max_v, color);
        setup_vertex(vertex2, rectangle.max_x, rectangle.max_y, max_u, max_v, color);
        setup_vertex(vertex3, rectangle.max_x, rectangle.min_y, max_u, min_v, color);
        
        tgui_u32 *index0 = tgui_array_push(index_buffer);
        tgui_u32 *index1 = tgui_array_push(index_buffer);
        tgui_u32 *index2 = tgui_array_push(index_buffer);

        *index0 = start_vertex_index + 0;
        *index1 = start_vertex_index + 1;
        *index2 = start_vertex_index + 2;

        tgui_u32 *index3 = tgui_array_push(index_buffer);
        tgui_u32 *index4 = tgui_array_push(index_buffer);
        tgui_u32 *index5 = tgui_array_push(index_buffer);

        *index3 = start_vertex_index + 2;
        *index4 = start_vertex_index + 3;
        *index5 = start_vertex_index + 0;
        
    
    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        tgui_u32 painter_w = tgui_rect_width(painter->dim);
        tgui_u32 *row = painter->pixels + (rectangle.min_y * painter_w) + rectangle.min_x;
        tgui_u32 *src_row = bitmap->pixels + (offset_y * bitmap->width) + offset_x;

        for(y = rectangle.min_y; y <= rectangle.max_y; ++y) {
            tgui_u32 *pixel = row;
            tgui_u32 *src_pixel = src_row;
            for(x = rectangle.min_x; x <= rectangle.max_x; ++x) {

                tgui_u32 src = *src_pixel;
                tgui_u32 des = *pixel;
                
                tgui_f32 tr = ((tint >> 16) & 0xff) / 255.0f;
                tgui_f32 tg = ((tint >>  8) & 0xff) / 255.0f;
                tgui_f32 tb = ((tint >>  0) & 0xff) / 255.0f;

                tgui_u32 sr = ((src >> 16) & 0xff) * tr;
                tgui_u32 sg = ((src >>  8) & 0xff) * tg;
                tgui_u32 sb = ((src >>  0) & 0xff) * tb;

                tgui_u32 dr = (des >> 16) & 0xff;
                tgui_u32 dg = (des >>  8) & 0xff;
                tgui_u32 db = (des >>  0) & 0xff;

                tgui_f32 sa = ((src >> 24) & 0xff) / 255.0f;
                
                tgui_u32 cr = (dr * (1.0f - sa) + sr * sa);
                tgui_u32 cg = (dg * (1.0f - sa) + sg * sa);
                tgui_u32 cb = (db * (1.0f - sa) + sb * sa);

                *pixel++ = (cr << 16) | (cg << 8) | (cb << 0);
                src_pixel++;
            }
            row += painter_w;
            src_row += bitmap->width;
        }

    } break;

    }
}

void tgui_painter_draw_vline(TGuiPainter *painter, tgui_s32 x, tgui_s32 y0, tgui_s32 y1, tgui_u32 color) {

    switch (painter->type) {
    case TGUI_PAINTER_TYPE_HARDWARE: {

        TGuiRectangle vline;
        vline.min_x = x;
        vline.max_x = x;
        vline.min_y = y0;
        vline.max_y = y1;
        tgui_painter_draw_rectangle(painter, vline, color);

    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        if(x < painter->clip.min_x)  return;
        if(x > painter->clip.max_x) return; 

        if(y0 < painter->clip.min_y)  y0 = painter->clip.min_y; 
        if(y1 > painter->clip.max_y)  y1 = painter->clip.max_y; 
        
        tgui_u32 painter_w = tgui_rect_width(painter->dim);
        tgui_u32 *pixel = painter->pixels + (y0 * painter_w) + x;
        
        for(tgui_s32 y = y0; y <= y1; ++y) {
            *pixel = color;
            pixel += painter_w;
        }

    } break;
    }
   

}

void tgui_painter_draw_hline(TGuiPainter *painter, tgui_s32 y, tgui_s32 x0, tgui_s32 x1, tgui_u32 color) {

    switch (painter->type) {
    case TGUI_PAINTER_TYPE_HARDWARE: {

        TGuiRectangle hline;
        hline.min_y = y;
        hline.max_y = y;
        hline.min_x = x0;
        hline.max_x = x1;
        tgui_painter_draw_rectangle(painter, hline, color);

    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {

        if(y < painter->clip.min_y)  return; 
        if(y > painter->clip.max_y)  return; 

        if(x0 < painter->clip.min_x) x0 = painter->clip.min_x; 
        if(x1 > painter->clip.max_x) x1 = painter->clip.max_x; 
        
        tgui_u32 painter_w = tgui_rect_width(painter->dim);
        tgui_u32 *pixel = painter->pixels + (y * painter_w) + x0;
        
        for(tgui_s32 x = x0; x <= x1; ++x) {
            *pixel++ = color;
        }

    } break;
    }
    

}

void tgui_painter_draw_render_buffer_texture(TGuiPainter *painter, TGuiRectangle dim) {

    TGuiRectangle rectangle = dim;

    clip_rectangle(&rectangle, painter->clip, 0, 0);

    switch (painter->type) {

    case TGUI_PAINTER_TYPE_HARDWARE: {
        
        if(tgui_rect_invalid(rectangle)) return;

        TGuiVertexArray *vertex_buffer = &painter->render_buffer->vertex_buffer;
        TGuiU32Array *index_buffer = &painter->render_buffer->index_buffer;

        rectangle.max_x += 1;
        rectangle.max_y += 1;

        tgui_u32 start_vertex_index = tgui_array_size(vertex_buffer);

        TGuiVertex *vertex0 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex1 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex2 = tgui_array_push(vertex_buffer);
        TGuiVertex *vertex3 = tgui_array_push(vertex_buffer);
        
        tgui_u32 color = 0xffffff;

        tgui_f32 min_u = 0.0f; 
        tgui_f32 min_v = 1.0f;
        tgui_f32 max_u = 1.0f; 
        tgui_f32 max_v = 0.0f;

        setup_vertex(vertex0, rectangle.min_x, rectangle.min_y, min_u, min_v, color);
        setup_vertex(vertex1, rectangle.min_x, rectangle.max_y, min_u, max_v, color);
        setup_vertex(vertex2, rectangle.max_x, rectangle.max_y, max_u, max_v, color);
        setup_vertex(vertex3, rectangle.max_x, rectangle.min_y, max_u, min_v, color);
        
        tgui_u32 *index0 = tgui_array_push(index_buffer);
        tgui_u32 *index1 = tgui_array_push(index_buffer);
        tgui_u32 *index2 = tgui_array_push(index_buffer);

        *index0 = start_vertex_index + 0;
        *index1 = start_vertex_index + 1;
        *index2 = start_vertex_index + 2;

        tgui_u32 *index3 = tgui_array_push(index_buffer);
        tgui_u32 *index4 = tgui_array_push(index_buffer);
        tgui_u32 *index5 = tgui_array_push(index_buffer);

        *index3 = start_vertex_index + 2;
        *index4 = start_vertex_index + 3;
        *index5 = start_vertex_index + 0;
        
    
    } break;
    case TGUI_PAINTER_TYPE_SOFTWARE: {
        TGUI_ASSERT(!"Invalid code path");
    } break;

    }

}

#if 0
void painter_draw_line(TGuiPainter *painter, tgui_s32 x0, tgui_s32 y0, tgui_s32 x1, tgui_s32 y1, tgui_u32 color) {
   
    // TODO: Do actual clipping with painter clip rectange

    tgui_s32 dx = x1 - x0;
    tgui_s32 dy = y1 - y0;

    if(dx == 0 && dy == y1) {
        painter_draw_pixel(painter, x0, x1, color);
        return;
    }

    if(dx == 0) {
        painter_draw_vline(painter, x0, y0, y1, color);
        return;
    }

    if(dy == 0) {
        painter_draw_hline(painter, y0, x0, x1, color);
        return;
    }

    TGUI_ASSERT(dx != 0);
    TGUI_ASSERT(dy != 0);
    
    tgui_s32 abs_dx = ABS(dx);
    tgui_s32 abs_dy = ABS(dy);

    if(abs_dx > abs_dy) {
        
        tgui_f32 x_advance = dx >= 0 ? 1 : -1;
        tgui_f32 y_advance = ((tgui_f32)dy / (tgui_f32)abs_dx);
        
        tgui_s32 x = x0;
        tgui_f32 y = y0;
        while(x != (x1 + x_advance)) {
            painter_draw_pixel(painter, x, (tgui_s32)y, color);
            x += x_advance;
            y += y_advance;
        }

    } else if(abs_dy > abs_dx) {

        tgui_f32 y_advance = dy >= 0 ? 1 : -1;
        tgui_f32 x_advance = ((tgui_f32)dx / (tgui_f32)abs_dy);
        
        tgui_s32 y = y0;
        tgui_f32 x = x0;
        while(y != (y1 + y_advance)) {
            painter_draw_pixel(painter, (tgui_s32)x, y, color);
            x += x_advance;
            y += y_advance;
        }

    } else {
        TGUI_ASSERT(abs_dx == abs_dy);
        tgui_f32 x_advance = dx >= 0 ? 1 : -1;
        tgui_f32 y_advance = dy >= 0 ? 1 : -1;

        tgui_s32 x = x0;
        tgui_s32 y = y0;
        while(x != (x1 + x_advance)) {
            painter_draw_pixel(painter, x, y, color);
            x += x_advance;
            y += y_advance;
        }
    } 

}
#endif
