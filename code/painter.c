#include "painter.h"

void painter_draw_pixel(Painter *painter, s32 x, s32 y, u32 color) {
    if(x >= painter->clip.min_x && x < painter->clip.max_x &&
       y >= painter->clip.min_y && y < painter->clip.max_y) {
        u32 painter_w = (painter->dim.max_x - painter->dim.min_x);
        u32 *pixel = painter->pixels + (y * painter_w) + x;
        *pixel = color;
    }
}

void painter_initialize(Painter *painter, u32 *pixels, Rectangle dim, Rectangle *clip) {
    
    painter->pixels = pixels;
    painter->dim = dim;

    if(clip) {
        painter->clip = *clip;
    } else {
        painter->clip = dim;
    }

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

void painter_draw_rect(Painter *painter, s32 x, s32 y, s32 w, s32 h, u32 color) {
    
    Rectangle rect;
    rect.min_x = x;
    rect.min_y = y;
    rect.max_x = x + w;
    rect.max_y = y + h;

    clip_rectangle(&rect, painter->clip, 0, 0);
    
    u32 painter_w = (painter->dim.max_x - painter->dim.min_x);
    u32 *row = painter->pixels + (rect.min_y * painter_w) + rect.min_x;

    for(y = rect.min_y; y < rect.max_y; ++y) {
        u32 *pixel = row;
        for(x = rect.min_x; x < rect.max_x; ++x) {
            *pixel++ = color;
        }
        row += painter_w;
    }
}

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, Bitmap *bitmap) {
    Rectangle rect;
    rect.min_x = x;
    rect.min_y = y;
    rect.max_x = x + bitmap->width;
    rect.max_y = y + bitmap->height;
    
    s32 offset_x;
    s32 offset_y;
    clip_rectangle(&rect, painter->clip, &offset_x, &offset_y);

    u32 painter_w = (painter->dim.max_x - painter->dim.min_x);
    u32 *row = painter->pixels + (rect.min_y * painter_w) + rect.min_x;
    u32 *src_row = bitmap->pixels + (offset_y * painter_w) + offset_x;

    for(y = rect.min_y; y < rect.max_y; ++y) {
        u32 *pixel = row;
        u32 *src_pixel = src_row;
        for(x = rect.min_x; x < rect.max_x; ++x) {
            *pixel++ = *src_pixel++;
        }
        row += painter_w;
        src_row += bitmap->width;
    }
}

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color) {
   
    if(x < painter->clip.min_x)  return;
    if(x >= painter->clip.max_x) return; 

    if(y0 < painter->clip.min_y) y0 = painter->clip.min_y; 
    if(y1 >= painter->clip.max_y) y1 = painter->clip.max_y - 1; 
    
    u32 painter_w = (painter->dim.max_x - painter->dim.min_x);
    u32 *pixel = painter->pixels + (y0 * painter_w) + x;
    
    for(s32 y = y0; y <= y1; ++y) {
        *pixel = color;
        pixel += painter_w;
    }

}

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color) {
    
    if(y < painter->clip.min_y)  return; 
    if(y >= painter->clip.max_y) return; 

    if(x0 < painter->clip.min_x) x0 = painter->clip.min_x; 
    if(x1 >= painter->clip.max_x) x1 = painter->clip.max_x - 1; 
    
    u32 painter_w = (painter->dim.max_x - painter->dim.min_x);
    u32 *pixel = painter->pixels + (y * painter_w) + x0;
    
    for(s32 x = x0; x <= x1; ++x) {
        *pixel++ = color;
    }

}

void painter_draw_line(Painter *painter, s32 x0, s32 y0, s32 x1, s32 y1, u32 color) {
   
    // TODO: Do actual clipping with painter clip rectange

    s32 dx = x1 - x0;
    s32 dy = y1 - y0;

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

    ASSERT(dx != 0);
    ASSERT(dy != 0);
    
    s32 abs_dx = ABS(dx);
    s32 abs_dy = ABS(dy);

    if(abs_dx > abs_dy) {
        
        f32 x_advance = dx >= 0 ? 1 : -1;
        f32 y_advance = ((f32)dy / (f32)abs_dx);
        
        s32 x = x0;
        f32 y = y0;
        while(x != (x1 + x_advance)) {
            painter_draw_pixel(painter, x, (s32)y, color);
            x += x_advance;
            y += y_advance;
        }

    } else if(abs_dy > abs_dx) {

        f32 y_advance = dy >= 0 ? 1 : -1;
        f32 x_advance = ((f32)dx / (f32)abs_dy);
        
        s32 y = y0;
        f32 x = x0;
        while(y != (y1 + y_advance)) {
            painter_draw_pixel(painter, (s32)x, y, color);
            x += x_advance;
            y += y_advance;
        }

    } else {
        ASSERT(abs_dx == abs_dy);
        f32 x_advance = dx >= 0 ? 1 : -1;
        f32 y_advance = dy >= 0 ? 1 : -1;

        s32 x = x0;
        s32 y = y0;
        while(x != (x1 + x_advance)) {
            painter_draw_pixel(painter, x, y, color);
            x += x_advance;
            y += y_advance;
        }
    } 

}
