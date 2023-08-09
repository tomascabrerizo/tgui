#include "painter.h"
#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "os.h"

static PainterFont painter_font;

void painter_initialize(Arena *arena) {
    //struct OsFont *font = os_font_create(arena, "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf", 20);
    struct OsFont *font = os_font_create(arena, "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf", 18);
    
    painter_font.glyph_rage_start = 32;
    painter_font.glyph_rage_end = 126;
    painter_font.glyph_count = (painter_font.glyph_rage_end - painter_font.glyph_rage_start + 1);
    painter_font.glyphs = arena_push_array(arena, PainterGlyph, painter_font.glyph_count, 8);
    os_font_get_vmetrics(font, &painter_font.ascent, &painter_font.descent, &painter_font.line_gap);

    void *temp_buffer = arena_alloc(arena, 512*512, 8);

    for(u32 glyph_index = painter_font.glyph_rage_start; glyph_index <= painter_font.glyph_rage_end; ++glyph_index) {
        
        s32 w, h, bpp;
        os_font_rasterize_glyph(font, glyph_index, &temp_buffer, &w, &h, &bpp);
    
        PainterGlyph *glyph = painter_font.glyphs + (glyph_index - painter_font.glyph_rage_start);
        glyph->bitmap.pixels = arena_alloc(arena, sizeof(u32)*w*h, 8);
        glyph->bitmap.width  = w;
        glyph->bitmap.height = h;
        
        memset(glyph->bitmap.pixels, 0, sizeof(u32)*w*h);
        
        u8 *src_row  = temp_buffer;
        u32 *des_row = glyph->bitmap.pixels;
        for(u32 y = 0; y < (u32)h; ++y) {
            u8 *src  = src_row;
            u32 *des = des_row;
            for(u32 x = 0; x < (u32)w; ++x) { 
                *des++ = 0x00ffffff | (*src++ << 24); 
            }
            src_row += w;
            des_row += w;
        }

        os_font_get_glyph_metrics(font, glyph_index, &glyph->adv_width, &glyph->left_bearing, &glyph->top_bearing);
    }
    
    painter_font.font = font;
}

void painter_terminate(void) {
    os_font_destroy(painter_font.font);
}

void painter_draw_pixel(Painter *painter, s32 x, s32 y, u32 color) {
    if(x >= painter->clip.min_x && x <= painter->clip.max_x &&
       y >= painter->clip.min_y && y <= painter->clip.max_y) {
        u32 painter_w = rect_width(painter->clip);
        u32 *pixel = painter->pixels + (y * painter_w) + x;
        *pixel = color;
    }
}

void painter_start(Painter *painter, u32 *pixels, Rectangle dim, Rectangle *clip) {
    
    painter->pixels = pixels;
    painter->dim = dim;

    if(clip) {
        painter->clip = *clip;
    } else {
        painter->clip = dim;
    }

}

void painter_draw_rectangle(Painter *painter, Rectangle rectangle, u32 color) {
    s32 x = rectangle.min_x;
    s32 y = rectangle.min_y;
    s32 w = rect_width(rectangle);
    s32 h = rect_height(rectangle);
    painter_draw_rect(painter, x, y, w, h, color);
}

void painter_draw_rectangle_outline(Painter *painter, Rectangle rectangle, u32 color) {
    painter_draw_hline(painter, rectangle.min_y, rectangle.min_x, rectangle.max_x, color);
    painter_draw_hline(painter, rectangle.max_y, rectangle.min_x, rectangle.max_x, color);
    painter_draw_vline(painter, rectangle.min_x, rectangle.min_y, rectangle.max_y, color);
    painter_draw_vline(painter, rectangle.max_x, rectangle.min_y, rectangle.max_y, color);
}

void painter_clear(Painter *painter, u32 color) {
    painter_draw_rectangle(painter, painter->clip, color);
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
    rect.max_x = x + w - 1;
    rect.max_y = y + h - 1;

    clip_rectangle(&rect, painter->clip, 0, 0);
    
    u32 painter_w = rect_width(painter->dim);
    u32 *row = painter->pixels + (rect.min_y * painter_w) + rect.min_x;

    for(y = rect.min_y; y <= rect.max_y; ++y) {
        u32 *pixel = row;
        for(x = rect.min_x; x <= rect.max_x; ++x) {
            *pixel++ = color;
        }
        row += painter_w;
    }
}

void painter_draw_bitmap(Painter *painter, s32 x, s32 y, Bitmap *bitmap, u32 tint) {
    Rectangle rect;
    rect.min_x = x;
    rect.min_y = y;
    rect.max_x = x + bitmap->width  - 1;
    rect.max_y = y + bitmap->height - 1;
    
    s32 offset_x;
    s32 offset_y;
    clip_rectangle(&rect, painter->clip, &offset_x, &offset_y);

    u32 painter_w = rect_width(painter->dim);
    u32 *row = painter->pixels + (rect.min_y * painter_w) + rect.min_x;
    u32 *src_row = bitmap->pixels + (offset_y * bitmap->width) + offset_x;

    for(y = rect.min_y; y <= rect.max_y; ++y) {
        u32 *pixel = row;
        u32 *src_pixel = src_row;
        for(x = rect.min_x; x <= rect.max_x; ++x) {

            u32 src = *src_pixel;
            u32 des = *pixel;
            
            f32 tr = ((tint >> 16) & 0xff) / 255.0f;
            f32 tg = ((tint >>  8) & 0xff) / 255.0f;
            f32 tb = ((tint >>  0) & 0xff) / 255.0f;

            u32 sr = ((src >> 16) & 0xff) * tr;
            u32 sg = ((src >>  8) & 0xff) * tg;
            u32 sb = ((src >>  0) & 0xff) * tb;

            u32 dr = (des >> 16) & 0xff;
            u32 dg = (des >>  8) & 0xff;
            u32 db = (des >>  0) & 0xff;

            f32 sa = ((src >> 24) & 0xff) / 255.0f;
            
            u32 cr = (dr * (1.0f - sa) + sr * sa);
            u32 cg = (dg * (1.0f - sa) + sg * sa);
            u32 cb = (db * (1.0f - sa) + sb * sa);

            *pixel++ = (cr << 16) | (cg << 8) | (cb << 0);
            src_pixel++;
        }
        row += painter_w;
        src_row += bitmap->width;
    }
}

void painter_draw_text(Painter *painter, s32 x, s32 y, char *text, u32 color) {
    
    s32 cursor = x;
    s32 base   = y + painter_font.ascent;

    u32 text_len = strlen(text);
    
    u32 last_index = 0; UNUSED(last_index);
    for(u32 i = 0; i < text_len; ++i) {
    
        u32 index = ((u32)text[i] - painter_font.glyph_rage_start);
       
        u32 kerning = 0;
#if 0
        /* TODO: look like kerning is always zero */
        if(last_index) {
            kerning = os_font_get_kerning_between(painter_font.font, last_index, index);
        }
#endif
        
        PainterGlyph *glyph = painter_font.glyphs + index;
        painter_draw_bitmap(painter, cursor + glyph->left_bearing - kerning, base - glyph->top_bearing, &glyph->bitmap, color);
        cursor += glyph->adv_width;

        last_index = index;
    }
}

Rectangle painter_get_text_dim(Painter *painter, s32 x, s32 y, char *text) {
    UNUSED(painter);
    Rectangle result;
    
    s32 w = 0;
    s32 h = painter_font.ascent - painter_font.descent + painter_font.line_gap;

    u32 text_len = strlen(text);
    for(u32 i = 0; i < text_len; ++i) {
        u32 index = ((u32)text[i] - painter_font.glyph_rage_start);
        PainterGlyph *glyph = painter_font.glyphs + index;
        w += glyph->adv_width;
    }

    result.min_x = x;
    result.min_y = y;
    result.max_x = result.min_x + w;
    result.max_y = result.min_y + h;

    return result;
}

void painter_draw_vline(Painter *painter, s32 x, s32 y0, s32 y1, u32 color) {
   
    if(x < painter->clip.min_x)  return;
    if(x > painter->clip.max_x) return; 

    if(y0 < painter->clip.min_y)  y0 = painter->clip.min_y; 
    if(y1 > painter->clip.max_y)  y1 = painter->clip.max_y; 
    
    u32 painter_w = rect_width(painter->dim);
    u32 *pixel = painter->pixels + (y0 * painter_w) + x;
    
    for(s32 y = y0; y <= y1; ++y) {
        *pixel = color;
        pixel += painter_w;
    }

}

void painter_draw_hline(Painter *painter, s32 y, s32 x0, s32 x1, u32 color) {
    
    if(y < painter->clip.min_y)  return; 
    if(y > painter->clip.max_y)  return; 

    if(x0 < painter->clip.min_x) x0 = painter->clip.min_x; 
    if(x1 > painter->clip.max_x) x1 = painter->clip.max_x; 
    
    u32 painter_w = rect_width(painter->dim);
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
