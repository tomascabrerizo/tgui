#include "tgui.h"
#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"
#include "os.h"
#include "tgui_docker.h"
#include "tgui_serializer.h"
#include <stdio.h>
#include <string.h>

/* -------------------------- */
/*       Hash Function     */
/* -------------------------- */

 static u64 murmur_hash64A(const void * key, int len, u64 seed) {
        const u64 m = 0xc6a4a7935bd1e995ull;
        const int r = 47;
        u64 h = seed ^ (len * m);
        
        const u64 * data = (const u64 *)key;
        const u64 * end = data + (len/8);
        
        while(data != end) {
            u64 k = *data++;

            k *= m; 
            k ^= k >> r; 
            k *= m; 
    
            h ^= k;
            h *= m; 
        }

        const unsigned char * data2 = (const unsigned char*)data;

        switch(len & 7) {
            case 7: h ^= (u64)(data2[6]) << 48; 
            case 6: h ^= (u64)(data2[5]) << 40;
            case 5: h ^= (u64)(data2[4]) << 32;
            case 4: h ^= (u64)(data2[3]) << 24;
            case 3: h ^= (u64)(data2[2]) << 16;
            case 2: h ^= (u64)(data2[1]) << 8;
            case 1: h ^= (u64)(data2[0]);
            h *= m;
        };
        
        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
} 

/* TODO: This global variables should be static */

TGui state;
TGuiInput input;
TGuiFont font;
extern TGuiDocker docker;


TGuiInput *tgui_get_input(void) {
    return &input;
}

TGuiCursor tgui_get_cursor_state(void) {
    return state.cursor;
}

u64 tgui_hash(void *bytes, u64 size) {
    return murmur_hash64A(bytes, size, 123);
}


/* ---------------------- */
/*       TGui Bitmap      */
/* ---------------------- */

TGuiBitmap tgui_bitmap_create_empty(u32 w, u32 h) {
    TGuiBitmap result;
    result.pixels = arena_alloc(&state.arena, w*h*sizeof(u32), 8);
    result.width  = w; 
    result.height = h;
    return result;
}

/* ---------------------- */
/*       TGui Font        */
/* ---------------------- */

static u32 get_codepoint_index(u32 codepoint) {
    
    if((codepoint < font.glyph_rage_start) || (codepoint > font.glyph_rage_end)) {
        codepoint = (u32)'?';
    }

    u32 index = (codepoint - font.glyph_rage_start);
    return index;
}

void tgui_font_initilize(Arena *arena) {
    
    struct OsFont *os_font = os_font_create(arena, "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf", 18);
    //struct OsFont *os_font = os_font_create(arena, "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf", 18);
    
    font.glyph_rage_start = 32;
    font.glyph_rage_end = 126;
    font.glyph_count = (font.glyph_rage_end - font.glyph_rage_start + 1);
    font.glyphs = arena_push_array(arena, TGuiGlyph, font.glyph_count, 8);
    os_font_get_vmetrics(os_font, &font.ascent, &font.descent, &font.line_gap);

    void *temp_buffer = arena_alloc(arena, 512*512, 8);

    for(u32 glyph_index = font.glyph_rage_start; glyph_index <= font.glyph_rage_end; ++glyph_index) {
        
        s32 w, h, bpp;
        os_font_rasterize_glyph(os_font, glyph_index, &temp_buffer, &w, &h, &bpp);
    
        TGuiGlyph *glyph = font.glyphs + (glyph_index - font.glyph_rage_start);
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

        os_font_get_glyph_metrics(os_font, glyph_index, &glyph->adv_width, &glyph->left_bearing, &glyph->top_bearing);
    }
    
    font.font = os_font;

    TGuiGlyph *default_glyph = font.glyphs + get_codepoint_index(' '); 
    font.max_glyph_width  = default_glyph->adv_width;
    font.max_glyph_height = font.ascent - font.descent + font.line_gap;

}

void tgui_font_terminate(void) {
    os_font_destroy(font.font);
}

TGuiGlyph *tgui_font_get_codepoint_glyph(u32 codepoint) {
    return font.glyphs + get_codepoint_index(codepoint);
}

Rectangle tgui_get_size_text_dim(s32 x, s32 y, char *text, u32 size) {
    Rectangle result;
    
    s32 w = 0;
    s32 h = font.max_glyph_height;

    u32 text_len = size;
    for(u32 i = 0; i < text_len; ++i) {
        TGuiGlyph *glyph = font.glyphs + get_codepoint_index(text[i]);
        w += glyph->adv_width;
    }

    result.min_x = x;
    result.min_y = y;
    result.max_x = result.min_x + w - 1;
    result.max_y = result.min_y + h - 1;

    return result;
}

Rectangle tgui_get_text_dim(s32 x, s32 y, char *text) {
    return tgui_get_size_text_dim(x, y, text, strlen(text));
}

void tgui_font_draw_text(Painter *painter, s32 x, s32 y, char *text, u32 size, u32 color) {
    s32 cursor = x;
    s32 base   = y + font.ascent;

    u32 text_len = size;
    
    u32 last_index = 0; UNUSED(last_index);
    for(u32 i = 0; i < text_len; ++i) {
        
        u32 index = get_codepoint_index(text[i]);

        TGuiGlyph *glyph = font.glyphs + index;
        painter_draw_bitmap(painter, cursor + glyph->left_bearing, base - glyph->top_bearing, &glyph->bitmap, color);
        cursor += glyph->adv_width;

        last_index = index;
    }
}

/* ---------------------- */
/*       TGui Widgets     */
/* ---------------------- */

TGuiAllocatedWindow *tgui_allocated_window_node_alloc(void) {
    TGuiAllocatedWindow *result = NULL;
    if(state.free_windows) {
        result = state.free_windows;
        state.free_windows = state.free_windows->next;
    } else {
        result = arena_push_struct(&state.arena, TGuiAllocatedWindow, 8);
    }
    ASSERT(result);
    memset(result, 0, sizeof(TGuiAllocatedWindow));

    return result;
}

void tgui_allocated_window_node_free(TGuiAllocatedWindow *allocated_window) {
    allocated_window->next = state.free_windows;
    state.free_windows = allocated_window;
}

TGuiWidget *tgui_widget_alloc(void) {
    TGuiWidget *widget = NULL;
    if(state.first_free_widget) {
        widget = state.first_free_widget;
        state.first_free_widget = state.first_free_widget->next;
    } else {
        widget = arena_push_struct(&state.arena, TGuiWidget, 8);
    }
    memset(widget, 0, sizeof(TGuiWidget));
    ASSERT(widget);
    return widget;
}

void tgui_widget_free(TGuiWidget *widget)  {
    widget->next = state.first_free_widget;
    state.first_free_widget = widget;
}

void tgui_window_process_widgets(TGuiWindow *window, Painter *painter) {
    TGuiWidget *widget = window->widgets->next;
    Rectangle saved_clip = painter->clip;
    while(!clink_list_end(widget, window->widgets)) {
        
        if(window->is_scrolling) {
            widget->x -= window->h_scroll_offset * (rect_width(window->scroll_saved_rect) - rect_width(window->dim));
            widget->y -= window->v_scroll_offset * (rect_height(window->scroll_saved_rect) - rect_height(window->dim));
        }

        painter->clip = rect_intersection(window->dim, painter->clip);
        widget->internal(widget, painter);
        painter->clip = saved_clip;
        widget = widget->next;
    
    }
}

void tgui_window_free_widgets(TGuiWindow *window) {
    TGuiWidget *widget = window->widgets->next;
    while(!clink_list_end(widget, window->widgets)) {
        TGuiWidget *to_free = widget;
        widget = widget->next;
        clink_list_remove(to_free);
        tgui_widget_free(to_free);
    }
}

void *_tgui_widget_get_state(u64 id, u64 size) {
    void *result = NULL;

    result = virtual_map_find(&state.registry, id);
    if(result == NULL) {
        result = arena_alloc(&state.arena, size, 8);
        memset(result, 0, size);
        virtual_map_insert(&state.registry, id, (void *)result);
    }

    ASSERT(result != NULL);
    return result;
}

b32 tgui_window_update_widget(TGuiWindow *window) {
    return tgui_docker_window_is_visible(window->parent, window);
}

static Rectangle calculate_widget_rect(TGuiWidget *widget) {

    TGuiWindow *window = widget->parent;
    s32 x = widget->x;
    s32 y = widget->y;
    s32 w = widget->w;
    s32 h = widget->h;
    
    Rectangle window_rect = window->dim;
    Rectangle rect = {
        window_rect.min_x + x,
        window_rect.min_y + y,
        window_rect.min_x + x + w - 1,
        window_rect.min_y + y + h - 1
    };

    return rect;
}

u64 tgui_get_widget_id(char *tgui_id) {
    u64 id = tgui_hash(tgui_id, strlen(tgui_id));
    return id;
}

void tgui_calculate_hot_widget(TGuiWindow *window, Rectangle rect, u64 id) {

    Rectangle visible_rect = rect_intersection(rect, window->dim);
    b32 mouse_is_over = rect_point_overlaps(visible_rect, input.mouse_x, input.mouse_y);
    
    if(mouse_is_over && !state.active) {
        state.hot = id;
    }

    if(!mouse_is_over && state.hot == id) {
        state.hot = 0;
    }

    if(docker.grabbing_window == true) {
        state.hot = 0;
    }
}

void tgui_widget_alloc_into_window(u64 id, TGuiWidgetInternalFunc internal, TGuiWindow *window, u32 x, u32 y, u32 w, u32 h) {
    
    TGuiWidget *widget = tgui_widget_alloc();
    
    widget->id = id;
    widget->parent = window;
    widget->x = x;
    widget->y = y;
    widget->w = w;
    widget->h = h;
    widget->next = 0;
    widget->prev = 0;

    widget->internal = internal;

    clink_list_insert_back(window->widgets, widget);
}

b32 _tgui_button(TGuiWindowHandle handle, char *label, s32 x, s32 y, char *tgui_id) {

    TGuiWindow *window = tgui_window_get_from_handle(handle);

    if(!tgui_window_update_widget(window)) {
        return false;
    }

    u64 id = tgui_get_widget_id(tgui_id);

    TGuiButton *button_state = tgui_widget_get_state(id, TGuiButton);
    button_state->label = label;
    
    tgui_widget_alloc_into_window(id, _tgui_button_internal, window, x, y, 120, 30);

    return button_state->result;
}

void _tgui_button_internal(TGuiWidget *widget, Painter *painter) {

    TGuiWindow *window = widget->parent;
    u64 id = widget->id;
    
    Rectangle rect = calculate_widget_rect(widget); 
    tgui_calculate_hot_widget(window, rect, id);

    b32 result = false;
    
    if(state.active == id) {
        if(!input.mouse_button_is_down && input.mouse_button_was_down) {
            if(state.hot == id) result = true;
            state.active = 0;
        }
    } else if(state.hot == id) {
        if(!input.mouse_button_was_down && input.mouse_button_is_down) {
            state.active = id;
        }    
    }

    /* TODO: Desing a command interface to render the complete UI independently */
    
    u32 button_color = 0x999999;
    u32 decoration_color = 0x444444;
    
    if(state.hot == id) {
        button_color = 0x888888;
    }

    if(state.active == id) {
        button_color = 0x777777;
    }
    
    /* TODO: All this rendering is temporal, the API should have a render independent way to give render primitives to the user */

    Rectangle saved_painter_clip = painter->clip;
    painter->clip = rect_intersection(rect, window->dim);
    
    painter_draw_rectangle(painter, rect, button_color);
    
    TGuiButton *button_state = tgui_widget_get_state(id, TGuiButton);
    
    char *label = button_state->label;
    Rectangle label_rect = tgui_get_text_dim(0, 0, label);
    
    s32 label_x = rect.min_x + (rect_width(rect) - 1) / 2 - (rect_width(label_rect) - 1) / 2;
    s32 label_y = rect.min_y + (rect_height(rect) - 1) / 2 - (rect_height(label_rect) - 1) / 2;
    tgui_font_draw_text(painter, label_x, label_y, label,  strlen(label), decoration_color);
    painter_draw_rectangle_outline(painter, rect, decoration_color);

    painter->clip = saved_painter_clip;
    
    button_state->result = result;
}

static Rectangle calculate_selection_rect(TGuiTextInput *text_input, s32 x, s32 y, u32 start, u32 end) {

    if(start > end) {
        u32 temp = start;
        start = end;
        end = temp;
    }
    ASSERT(start <= end);

    Rectangle result = {
        x + (start - text_input->offset) * font.max_glyph_width,
        y,
        x + (end - text_input->offset) * font.max_glyph_width,
        y + font.max_glyph_height,
    };

    return result;
}

static void delete_selection(TGuiTextInput *text_input) {
    
    u32 start = text_input->selection_start;
    u32 end = text_input->selection_end;

    if(start > end) {
        u32 temp = start;
        start = end;
        end = temp;
    }
    ASSERT(start <= end);

    ASSERT(text_input->selection == true);
    memmove(text_input->buffer + start,
            text_input->buffer + end,
            text_input->used - end);
    
    ASSERT((end - start) <= text_input->used);
    
    text_input->used -= (end - start);
    text_input->selection = false;
    text_input->cursor = start;
    if(text_input->offset > start) {
        text_input->offset = start;
    }
}

TGuiTextInput *_tgui_text_input(TGuiWindowHandle handle, s32 x, s32 y, char *tgui_id) {
    
    TGuiWindow *window = tgui_window_get_from_handle(handle);

    u64 id = tgui_get_widget_id(tgui_id);

    TGuiTextInput *text_input = tgui_widget_get_state(id, TGuiTextInput);

    if(!tgui_window_update_widget(window)) {
        if(state.active == id) {
            state.active = 0;
        }
        return text_input;
    }
    
    tgui_widget_alloc_into_window(id, _tgui_text_input_internal, window, x, y, 140, 30);

    return text_input; 
}

void _tgui_text_input_internal(TGuiWidget *widget, Painter *painter) {

    TGuiWindow *window = widget->parent;
    u64 id = widget->id;
    
    TGuiKeyboard *keyboard = &input.keyboard;

    Rectangle rect = calculate_widget_rect(widget);
    tgui_calculate_hot_widget(window, rect, id);
    
    TGuiTextInput *text_input = tgui_widget_get_state(id, TGuiTextInput);
    
    if(!text_input->initilize) {
        
        text_input->selection = false;
        text_input->selection_start = 0;
        text_input->selection_end = 0;
        
        text_input->cursor_inactive_acumulator = 0;
        text_input->cursor_inactive_target = 0.5f;
        text_input->cursor_blink_target = 0.4f; 
        text_input->cursor_blink_acumulator = 0; 
        text_input->blink_cursor = false;
        text_input->draw_cursor  = true;

        text_input->initilize = true;
    }
    
    text_input->cursor_inactive_acumulator += state.dt;
    if(text_input->cursor_inactive_acumulator >= text_input->cursor_inactive_target) {
        text_input->blink_cursor = true;
    }

    if(text_input->blink_cursor) {
        text_input->cursor_blink_acumulator += state.dt;
        if(text_input->cursor_blink_acumulator >= text_input->cursor_blink_target) {
            text_input->draw_cursor = !text_input->draw_cursor;
            text_input->cursor_blink_acumulator = 0;
        }
    }

    Rectangle visible_rect = rect_intersection(rect, window->dim);
    u32 padding_x = 8;
    u32 visible_glyphs = MAX((s32)((rect_width(visible_rect) - padding_x*2)/font.max_glyph_width), (s32)0);

    if(state.active == id) {
        
        if(keyboard->k_r_arrow_down) {
            
            /* NOTE: Not blick cursor */
            text_input->blink_cursor = false;
            text_input->draw_cursor = true;
            text_input->cursor_inactive_acumulator = 0;

            /* NOTE: Start selection */
            if(!keyboard->k_shift) {
                text_input->selection = false;
            } else if(keyboard->k_shift && !text_input->selection) {
                text_input->selection = true;
                text_input->selection_start = text_input->cursor;
            }

            text_input->cursor = MIN(text_input->cursor + 1, text_input->used);

            /* NOTE: End selection */
            if(text_input->selection) {
                text_input->selection_end = text_input->cursor;
                ASSERT(ABS((s32)text_input->selection_end - (s32)text_input->selection_start) <= (s32)text_input->used);
            }
  
        } else if(keyboard->k_l_arrow_down) {

            /* NOTE: Not blick cursor */
            text_input->blink_cursor = false;
            text_input->draw_cursor = true;
            text_input->cursor_inactive_acumulator = 0;
        
            /* NOTE: Start selection */
            if(!keyboard->k_shift) {
                text_input->selection = false;
            } else if(keyboard->k_shift && !text_input->selection) {
                text_input->selection = true;
                text_input->selection_start = text_input->cursor;
            }
            
            text_input->cursor = MAX((s32)text_input->cursor - 1, 0);

            /* NOTE: End selection */
            if(text_input->selection) {
                text_input->selection_end = text_input->cursor;
                ASSERT(ABS((s32)text_input->selection_end - (s32)text_input->selection_start) <= (s32)text_input->used);
            }
        
        } else if(keyboard->k_backspace) {

            /* NOTE: Not blick cursor */
            text_input->blink_cursor = false;
            text_input->draw_cursor = true;
            text_input->cursor_inactive_acumulator = 0;
            
            if(text_input->selection) {
                delete_selection(text_input);
            } else if(text_input->cursor > 0) {
                memmove(text_input->buffer + text_input->cursor - 1,
                        text_input->buffer + text_input->cursor,
                        text_input->used - text_input->cursor);
                text_input->cursor -= 1;
                text_input->used   -= 1;
            }
        
        } else if(keyboard->k_delete) {

            /* NOTE: Not blick cursor */
            text_input->blink_cursor = false;
            text_input->draw_cursor = true;
            text_input->cursor_inactive_acumulator = 0;

            if(text_input->selection) {
                delete_selection(text_input);
            } else if(text_input->cursor < text_input->used) {
                memmove(text_input->buffer + text_input->cursor,
                        text_input->buffer + text_input->cursor + 1,
                        text_input->used - text_input->cursor - 1);
                text_input->used -= 1;
            }

        } else if(input.text_size > 0) {

            /* NOTE: Not blick cursor */
            text_input->blink_cursor = false;
            text_input->draw_cursor = true;
            text_input->cursor_inactive_acumulator = 0;

            if(text_input->selection) {
                delete_selection(text_input);
            }

            if((text_input->used + input.text_size) > TGUI_TEXT_INPUT_MAX_CHARACTERS) {
                input.text_size = (TGUI_TEXT_INPUT_MAX_CHARACTERS - text_input->used);
                ASSERT((text_input->used + input.text_size) == TGUI_TEXT_INPUT_MAX_CHARACTERS);
            }

            memmove(text_input->buffer + text_input->cursor + input.text_size, 
                    text_input->buffer + text_input->cursor, 
                    (text_input->used - text_input->cursor));
            memcpy(text_input->buffer + text_input->cursor, input.text, input.text_size);

            text_input->used   += input.text_size;
            text_input->cursor += input.text_size;
        }
        
        /* Calculate the ofset of the text */
        if(text_input->cursor > text_input->offset + visible_glyphs) {
            text_input->offset += 1;
        } else if(text_input->offset > 0 && text_input->cursor < text_input->offset) {
            text_input->offset -= 1;
        }
    
    } else if(state.hot == id) {
        if(!input.mouse_button_was_down && input.mouse_button_is_down) {
            state.active = id;
            /* TODO: Find a better aproach to handle text input */
            input.text_size = 0;
        }    
    }

    b32 mouse_is_over = rect_point_overlaps(visible_rect, input.mouse_x, input.mouse_y);
    if(state.active == id && !mouse_is_over && input.mouse_button_was_down && !input.mouse_button_is_down) {
        state.active = 0;
    }
 
    Rectangle saved_painter_clip = painter->clip;
    painter->clip = window->dim; 
    
    u32 color = 0x888888;
    u32 decoration_color = 0x333333;
    u32 cursor_color = 0x00ff00;
    
    if(state.hot == id) {
        color = 0x888888;
    }
    
    if(state.active == id) {
        color = 0x999999;
        decoration_color = 0x333333;
    }

    painter_draw_rectangle(painter, rect, color);
    painter_draw_rectangle_outline(painter, rect, decoration_color);

    Rectangle clipping_rect = rect;
    clipping_rect.min_x += padding_x;
    clipping_rect.max_x -= padding_x;
    painter->clip = rect_intersection(clipping_rect, painter->clip);
    
    s32 text_x = rect.min_x + padding_x;
    s32 text_y = rect.min_y + ((rect_height(rect) - 1) / 2) - ((font.max_glyph_height - 1) / 2);
    

    if(text_input->selection) {
        Rectangle selection_rect = calculate_selection_rect(text_input, text_x, text_y, text_input->selection_start, text_input->selection_end);
        painter_draw_rectangle(painter, selection_rect, 0x7777ff);
    }

    tgui_font_draw_text(painter, text_x, text_y, (char *)text_input->buffer + text_input->offset,
            text_input->used - text_input->offset, decoration_color);

    if(state.active == id && text_input->draw_cursor) {
        Rectangle cursor_rect = {
            text_x + ((text_input->cursor - text_input->offset) * font.max_glyph_width),
            text_y,
            text_x + ((text_input->cursor - text_input->offset) * font.max_glyph_width),
            text_y + font.max_glyph_height,
        };
        painter_draw_rectangle(painter, cursor_rect, cursor_color);
    }
    
    painter->clip = saved_painter_clip;

}

static u32 u32_color_mix(u32 color0, f32 t, u32 color1) {

    u32 r0 = ((color0 >> 16) & 0xff);
    u32 g0 = ((color0 >>  8) & 0xff);
    u32 b0 = ((color0 >>  0) & 0xff);

    u32 r1 = ((color1 >> 16) & 0xff);
    u32 g1 = ((color1 >>  8) & 0xff);
    u32 b1 = ((color1 >>  0) & 0xff);

    u32 cr = (r0 * (1.0f - t) + r1 * t);
    u32 cg = (g0 * (1.0f - t) + g1 * t);
    u32 cb = (b0 * (1.0f - t) + b1 * t);
    
    return (cr << 16) | (cg << 8) | (cb << 0);
}

static void colorpicker_calculate_radiant(TGuiBitmap *bitmap, u32 color) {
    
    u32 w = (bitmap->width - 1) != 0 ? (bitmap->width - 1) : 1;
    f32 inv_w = 1.0f / w;

    u32 *pixel = bitmap->pixels;
    for(u32 x = 0; x < bitmap->width; ++x) { 
        f32 t = (f32)x * inv_w;
        *pixel++ = u32_color_mix(0xffffff, t, color);
    }
    
    u32 h = (bitmap->height - 1) != 0 ? (bitmap->height - 1) : 1;
    f32 inv_h = 1.0f / h;
    
    for(u32 x = 0; x < bitmap->width; ++x) {
        u32 color = bitmap->pixels[x];
        for(u32 y = 1; y < bitmap->height; ++y) {
            f32 t = (f32)y * inv_h;
            bitmap->pixels[y*bitmap->width+x] = u32_color_mix(color, t, 0x000000);
        }
    } 


}

static void colorpicker_claculate_section(TGuiBitmap *bitmap, u32 *cursor_x, u32 advance, u32 color0, u32 color1) {

    f32 inv_advance = advance != 0 ? (1.0f / advance) : 1.0f;

    for(u32 y = 0; y < bitmap->height; ++y) {
        u32 xx = *cursor_x;
        for(u32 x = 0; x < advance; ++x) {
            f32 t = x * inv_advance;
            bitmap->pixels[y*bitmap->width+xx++] = u32_color_mix(color0, t, color1);
        }
    }

    *cursor_x += advance;
}

static void colorpicker_calculate_mini_radiant(TGuiBitmap *bitmap) {
    
    u32 advance = 0;
    u32 cursor_x = 0;
    
    u32 color0 = 0xff0000;
    u32 color1 = 0xffff00;
    advance = (bitmap->width * 1.0f/6.0f);
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);
    
    color0 = 0xffff00;
    color1 = 0x00ff00;
    advance = (bitmap->width * 2.0f/6.0f);
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);
    
    color0 = 0x00ff00;
    color1 = 0x00ffff;
    advance = (bitmap->width * 3.0f/6.0f);
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);
    
    color0 = 0x00ffff;
    color1 = 0x0000ff;
    advance = (bitmap->width * 4.0f/6.0f);
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);
    
    color0 = 0x0000ff;
    color1 = 0xff00ff;
    advance = (bitmap->width * 5.0f/6.0f);
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);
    
    color0 = 0xff00ff;
    color1 = 0xff0000;
    advance = bitmap->width;
    colorpicker_claculate_section(bitmap, &cursor_x, advance - cursor_x, color0, color1);

}

void tgui_u32_color_to_hsv_color(u32 color, f32 *h, f32 *s, f32 *v) {
    
    s32 r = ((color >> 16) & 0xff);// / 255.0f;
    s32 g = ((color >>  8) & 0xff);// / 255.0f;
    s32 b = ((color >>  0) & 0xff);// / 255.0f;

    s32 M = MAX3(r, g, b);
    s32 m = MIN3(r, g, b);
    f32 chroma = M - m;
    
    f32 hue = 0;
    if(chroma > 0.00001f) {
        if(M <= r) {
            hue = fmod((g-b)/chroma, 6);
        } else if(M <= g) {
            hue = 2.0f + (b-r)/chroma;
        } else if(M <= b) {
            hue = 4.0f + (r-g)/chroma;
        }
    }
    hue *= 60;
    if(hue < 0) hue += 360;

    f32 saturation = 0;
    if(M != 0) {
        saturation = chroma / (f32)M;
    }
    f32 value = M / 255.0f;

    *h = hue / 360.0f;
    *s = saturation;
    *v = value;
}

static u32 colorpicker_get_color_hue(TGuiColorPicker *colorpicker) {
    return colorpicker->mini_radiant.pixels[(u32)(colorpicker->hue * (colorpicker->mini_radiant.width - 0))];
}

void _tgui_color_picker(TGuiWindowHandle handle, s32 x, s32 y, s32 w, s32 h, u32 *color, char *tgui_id) {

    TGuiWindow *window = tgui_window_get_from_handle(handle);

    if(!tgui_window_update_widget(window)) {
        return;
    }
    u64 id = tgui_get_widget_id(tgui_id);
    
    TGuiColorPicker *colorpicker = tgui_widget_get_state(id, TGuiColorPicker);
    colorpicker->color_ptr = color;
    
    tgui_widget_alloc_into_window(id, _tgui_color_picker_internal, window, x, y, w, h);

}

void _tgui_color_picker_internal(TGuiWidget *widget, Painter *painter) {
    
    TGuiWindow *window = widget->parent;
    u64 id = widget->id;
    u32 w = widget->w;
    u32 h = widget->h;
    
    Rectangle rect = calculate_widget_rect(widget);
    tgui_calculate_hot_widget(window, rect, id);

    TGuiColorPicker *colorpicker = tgui_widget_get_state(id, TGuiColorPicker);

    /* TODO: Actually use hue saturation and value from the color */
    if(colorpicker->color_ptr) {
        f32 hue, saturation, value;
        tgui_u32_color_to_hsv_color(*colorpicker->color_ptr, &hue, &saturation, &value);
    }

    f32 radiant_h = h * 0.75f; 
    f32 mini_radiant_h = h * 0.2f;
    
    if(!colorpicker->initialize) {
         
        colorpicker->mini_radiant = tgui_bitmap_create_empty(w, mini_radiant_h);
        colorpicker_calculate_mini_radiant(&colorpicker->mini_radiant);
        
        colorpicker->saved_radiant_color = colorpicker_get_color_hue(colorpicker);
        colorpicker->radiant = tgui_bitmap_create_empty(w, radiant_h);
        colorpicker_calculate_radiant(&colorpicker->radiant, colorpicker->saved_radiant_color);
        
        colorpicker->sv_cursor_active = false;
        colorpicker->hue_cursor_active = false;

        colorpicker->initialize = true;
    }

    Rectangle radiant_rect = rect_from_wh(rect.min_x, rect.min_y, colorpicker->radiant.width, colorpicker->radiant.height);
    Rectangle mini_radiant_rect = rect_from_wh(rect.min_x, rect.max_y - mini_radiant_h, colorpicker->mini_radiant.width, colorpicker->mini_radiant.height);

    if(state.hot == id) {
        b32 mouse_is_over = rect_point_overlaps(mini_radiant_rect, input.mouse_x, input.mouse_y); 
        if(mouse_is_over && input.mouse_button_is_down && !input.mouse_button_was_down) {
            state.active = id;
            colorpicker->hue_cursor_active = true;
        }

        mouse_is_over = rect_point_overlaps(radiant_rect, input.mouse_x, input.mouse_y); 
        if(mouse_is_over && input.mouse_button_is_down && !input.mouse_button_was_down) {
            state.active = id;
            colorpicker->sv_cursor_active = true;
        }
    }
    
    if(state.active == id && input.mouse_button_was_down && !input.mouse_button_is_down) {
        state.active = 0;
        colorpicker->sv_cursor_active = false;
        colorpicker->hue_cursor_active = false;
    }

    if(colorpicker->hue_cursor_active) {
        colorpicker->hue = CLAMP((input.mouse_x - mini_radiant_rect.min_x) / (f32)colorpicker->mini_radiant.width, 0, 1);
    }

    if(colorpicker->sv_cursor_active) {
        colorpicker->saturation =  CLAMP((input.mouse_x - radiant_rect.min_x) / (f32)colorpicker->radiant.width, 0, 1);
        colorpicker->value      =  CLAMP((input.mouse_y - radiant_rect.min_y) / (f32)colorpicker->radiant.height, 0, 1);
    }

    u32 cursor_w = 6;
    Rectangle hue_cursor = mini_radiant_rect;

    hue_cursor.min_x += (colorpicker->hue * (colorpicker->mini_radiant.width - cursor_w)); 
    hue_cursor.max_x = hue_cursor.min_x + cursor_w - 1;  

    u32 radiant_color = colorpicker_get_color_hue(colorpicker);
    if(colorpicker->saved_radiant_color != radiant_color) {
        colorpicker_calculate_radiant(&colorpicker->radiant, radiant_color);
        colorpicker->saved_radiant_color = radiant_color;
    }
    
    Rectangle saved_clip = painter->clip;
    painter->clip = window->dim;

    u32 color_x = colorpicker->saturation * (colorpicker->radiant.width -  1);
    u32 color_y = colorpicker->value * (colorpicker->radiant.height - 1);
    
    painter_draw_bitmap_no_alpha(painter, radiant_rect.min_x, radiant_rect.min_y, &colorpicker->radiant);
    painter_draw_rectangle_outline(painter, radiant_rect, 0x444444);
    
    painter_draw_hline(painter, radiant_rect.min_y + color_y, radiant_rect.min_x, radiant_rect.max_x, 0x444444);
    painter_draw_vline(painter, radiant_rect.min_x + color_x, radiant_rect.min_y, radiant_rect.max_y, 0x444444);
    
    painter_draw_bitmap_no_alpha(painter, mini_radiant_rect.min_x, mini_radiant_rect.min_y, &colorpicker->mini_radiant);
    painter_draw_rectangle_outline(painter, mini_radiant_rect, 0x444444);
    
    painter->clip = rect_intersection(window->dim, mini_radiant_rect);

    painter_draw_rectangle(painter, hue_cursor, 0x444444);
    painter_draw_rectangle_outline(painter, hue_cursor, 0x888888);

    painter->clip = saved_clip;
    
    if(colorpicker->color_ptr) {
        *colorpicker->color_ptr = colorpicker->radiant.pixels[color_y*colorpicker->radiant.width + color_x];
    }
}

TGuiTreeViewNode *treeview_alloc_node(TGuiTreeView *treeview) {
    TGuiTreeViewNode *node = NULL;
    if(treeview->first_free_node) {
        node = treeview->first_free_node;
        treeview->first_free_node = treeview->first_free_node->next;
    } else {
        node = arena_push_struct(&state.arena, TGuiTreeViewNode, 8);
    }
    ASSERT(node);
    memset(node, 0, sizeof(TGuiTreeViewNode));
    return node;
}

void treeview_free_node(TGuiTreeView *treeview, TGuiTreeViewNode *node) {
    if(node->childs) {
        
        TGuiTreeViewNode *child = node->childs->next;
        while(!clink_list_end(child, node->childs)) {
            
            TGuiTreeViewNode *to_free = child;
            child = child->next;

            treeview_free_node(treeview, to_free);

        }

        treeview_free_node(treeview, node->childs);

    } else {
        node->next = treeview->first_free_node;
        treeview->first_free_node = node; 
    }
}

TGuiTreeViewNode *treeview_alloc_root_node(TGuiTreeView *treeview) {
    TGuiTreeViewNode *node = treeview_alloc_node(treeview);
    node->childs = treeview_alloc_node(treeview);
    clink_list_init(node->childs);
    return node;
}

void treeview_insert_node(TGuiTreeViewNode *node, TGuiTreeViewNode *parent) {
    clink_list_insert_back(parent->childs, node);
    node->parent = parent;
}


void _tgui_tree_view_begin(TGuiWindowHandle handle, char *tgui_id) {

    TGuiWindow *window = tgui_window_get_from_handle(handle);

    u32 id = tgui_get_widget_id(tgui_id);

    state.active_id = id;
    state.active_window = window;
    
    if(!tgui_window_update_widget(window)) {
        return;
    }
    
    TGuiTreeView *treeview = tgui_widget_get_state(state.active_id, TGuiTreeView);
    treeview->dim = (Rectangle){ 0, 0, 0, 0 };
    treeview->dim.max_x = rect_width(window->dim)-1;

    if(!treeview->initiliaze) {

        treeview->first_free_node = 0;
        
        for(u32 i = 0; i < TGUI_TREEVIEW_MAX_STACK_SIZE; ++i) {
            treeview->root_node_state[i] = true;
        }
        treeview->root_node_state_head = 0;

        treeview->initiliaze = true;
    }

    if(treeview->root) {
        treeview_free_node(treeview, treeview->root);
    }

    treeview->root = treeview_alloc_root_node(treeview);
    treeview->root->label = "dummy root node";

    treeview->active_root_node = treeview->root;
    treeview->root_node_state_head = 0;
    treeview->active_depth = 0;
}

void _tgui_tree_view_end(void) {

    if(!tgui_window_update_widget(state.active_window)) {
        return;
    }

    TGuiTreeView *treeview = tgui_widget_get_state(state.active_id, TGuiTreeView);
    s32 x = treeview->dim.min_x;
    s32 y = treeview->dim.min_y;
    s32 w = rect_width(treeview->dim);
    s32 h = rect_height(treeview->dim);

    tgui_widget_alloc_into_window(state.active_id, _tgui_tree_view_internal, state.active_window, x, y, w, h);

    state.active_window = NULL;
    state.active_id = -1;
}

void treeview_node_setup(TGuiTreeView *treeview, TGuiTreeViewNode *node, char *label, s32 depth) {
    UNUSED(treeview);

    node->label = label;
    if(node->childs) {
        node->state_index = treeview->root_node_state_head++;
        ASSERT(treeview->root_node_state_head <= TGUI_TREEVIEW_MAX_STACK_SIZE);
    }

    node->label_depth = depth;
    
    if((node->parent && node->parent == treeview->root) || (node->parent && treeview->root_node_state[node->parent->state_index] && node->parent->visible)){
        Rectangle text_label = tgui_get_text_dim(depth*TGUI_TREEVIEW_DEFAULT_DEPTH_WIDTH, treeview->dim.max_y+1, label);
        treeview->dim = rect_union(treeview->dim, text_label);

        node->dim = text_label;
        node->dim.min_x = treeview->dim.min_x;
        node->dim.max_x = treeview->dim.max_x;
        
        node->visible = true;
    } else {
        node->visible = false;
    }
}

void _tgui_tree_view_root_node_begin(char *label) {
    if(!tgui_window_update_widget(state.active_window)) {
        return;
    }

    
    TGuiTreeView *treeview = tgui_widget_get_state(state.active_id, TGuiTreeView);
   
    TGuiTreeViewNode *node = treeview_alloc_root_node(treeview);
    treeview_insert_node(node, treeview->active_root_node);
    treeview_node_setup(treeview, node, label, treeview->active_depth);
    
    treeview->active_root_node = node;
    treeview->active_depth += 1;
}

void _tgui_tree_view_root_node_end(void) {
    if(!tgui_window_update_widget(state.active_window)) {
        return;
    }

    TGuiTreeView *treeview = tgui_widget_get_state(state.active_id, TGuiTreeView);
    treeview->active_root_node = treeview->active_root_node->parent;
    treeview->active_depth -= 1;
}

void _tgui_tree_view_node(char *label) {

    if(!tgui_window_update_widget(state.active_window)) {
        return;
    }
    
    TGuiTreeView *treeview = tgui_widget_get_state(state.active_id, TGuiTreeView);

    TGuiTreeViewNode *node = treeview_alloc_node(treeview);
    treeview_insert_node(node, treeview->active_root_node);
    treeview_node_setup(treeview, node, label, treeview->active_depth);

}

void tree_view_translate_node_dim(TGuiTreeView *treeview, TGuiTreeViewNode *node) {

    s32 x = node->dim.min_x;
    s32 y = node->dim.min_y;
    s32 w = rect_width(treeview->dim);
    s32 h = rect_height(node->dim);
    
    Rectangle treeview_rect = treeview->dim; 
    Rectangle result = {
        treeview_rect.min_x + x,
        treeview_rect.min_y + y,
        treeview_rect.min_x + x + w - 1,
        treeview_rect.min_y + y + h - 1
    };

    node->dim = result;
}

void treeview_node_draw(TGuiTreeView *treeview, TGuiTreeViewNode *node, Painter *painter, u32 *color) {
    if(!node->visible) return;

    if(*color == TGUI_TREEVIEW_COLOR0) {
        *color = TGUI_TREEVIEW_COLOR1;
    } else {
        *color = TGUI_TREEVIEW_COLOR0;
    }

    painter_draw_rectangle(painter, node->dim, *color);
    s32 label_x = node->dim.min_x+node->label_depth*TGUI_TREEVIEW_DEFAULT_DEPTH_WIDTH;
    tgui_font_draw_text(painter, label_x, node->dim.min_y, node->label, strlen(node->label), 0x333333);

    if(node->childs) {

        TGuiTreeViewNode *child = node->childs->next;
        while(!clink_list_end(child, node->childs)) {
            treeview_node_draw(treeview, child, painter, color);
            child = child->next;
        }
    }
    
}

void tree_view_node_draw(TGuiTreeView *treeview, TGuiTreeViewNode *node, Painter *painter, u32 *color) {

    if(*color == TGUI_TREEVIEW_COLOR0) {
        *color = TGUI_TREEVIEW_COLOR1;
    } else {
        *color = TGUI_TREEVIEW_COLOR0;
    }
    
    if(node == treeview->root) {

        if(node->childs) {

            TGuiTreeViewNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                tree_view_node_draw(treeview, child, painter, color);
                child = child->next;
            }
        }

    } else {
        painter_draw_rectangle(painter, node->dim, *color);
        s32 label_x = node->dim.min_x+node->label_depth*TGUI_TREEVIEW_DEFAULT_DEPTH_WIDTH;
        tgui_font_draw_text(painter, label_x, node->dim.min_y, node->label, strlen(node->label), 0x333333);

        if(node->childs && treeview->root_node_state[node->state_index] && node->visible) {

            TGuiTreeViewNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                tree_view_node_draw(treeview, child, painter, color);
                child = child->next;
            }
        }

    }
}

void treeview_translate_node(TGuiTreeView *treeview, TGuiTreeViewNode *node) {
        if(!node->visible) return;
        
        tree_view_translate_node_dim(treeview, node);

        if(node->childs) {
            TGuiTreeViewNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                treeview_translate_node(treeview, child);
                child = child->next;
            }
        }
}

void treeview_update_node(TGuiTreeView *treeview, TGuiTreeViewNode *node) {
        if(!node->visible) return;

        if(node->childs && rect_point_overlaps(node->dim, input.mouse_x, input.mouse_y)) {
           if(input.mouse_button_was_down && !input.mouse_button_is_down) {
                treeview->root_node_state[node->state_index] = !treeview->root_node_state[node->state_index];
           }
        }

        if(node->childs) {
            TGuiTreeViewNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                treeview_update_node(treeview, child);
                child = child->next;
            }
        }
}

void _tgui_tree_view_internal(TGuiWidget *widget, Painter *painter) {

    TGuiWindow *window = widget->parent;
    u64 id = widget->id;

    TGuiTreeView *treeview = tgui_widget_get_state(id, TGuiTreeView);
    treeview->dim = calculate_widget_rect(widget); 

    tgui_calculate_hot_widget(window, treeview->dim, id);
    
    /* NOTE: Translate node to window */
    TGuiTreeViewNode *node = treeview->root->childs->next;
    while(!clink_list_end(node, treeview->root->childs)) {
        treeview_translate_node(treeview, node);
        node = node->next; 
    }

    /* NOTE: Draw node */
    //Rectangle saved_painter_clip = painter->clip;
    //painter->clip = rect_intersection(treeview->dim, window->dim);
    //u32 color = TGUI_TREEVIEW_COLOR0;
    //TGuiTreeViewNode *root = treeview->root;
    //tree_view_node_draw(treeview, root, painter, &color);
    //painter->clip = saved_painter_clip;

    u32 color = TGUI_TREEVIEW_COLOR0;
    node = treeview->root->childs->next;
    while(!clink_list_end(node, treeview->root->childs)) {
        treeview_node_draw(treeview, node, painter, &color);
        node = node->next; 
    }

    /* NOTE: Update node state */
    node = treeview->root->childs->next;
    while(!clink_list_end(node, treeview->root->childs)) {
        treeview_update_node(treeview, node);
        node = node->next; 
    }
    




}

/* ---------------------- */
/*       TGui Window      */
/* ---------------------- */

TGuiWindow *tgui_window_alloc(TGuiDockerNode *parent, char *name, b32 scroll, TGuiAllocatedWindow *list) {

    TGuiAllocatedWindow *allocated_window_node = tgui_allocated_window_node_alloc();
    clink_list_insert_back(list, allocated_window_node);

    TGuiWindow *window = &allocated_window_node->window;
    ASSERT(window);

    window->id = state.window_id_generator++;
    tgui_docker_window_node_add_window(parent, window);
    window->name =  name;
    parent->active_window = window->id;
    
    window->widgets = tgui_widget_alloc();
    clink_list_init(window->widgets);

    window->is_scrolling = scroll;

    window->h_scroll_bar = rect_set_invalid();
    window->v_scroll_bar = rect_set_invalid();
    
    return window;
}

TGuiWindow *tgui_window_get_from_handle(TGuiWindowHandle handle) {

    TGuiAllocatedWindow *allocated_window = state.allocated_windows->next;
    while(!clink_list_end(allocated_window, state.allocated_windows)) {
        TGuiWindow *window = &allocated_window->window;
        if(window->id == handle) return window;
        allocated_window = allocated_window->next;
    }
    ASSERT(!"Invalid code path");
    return NULL;
}

TGuiWindowHandle tgui_create_root_window(char *name, b32 scroll) {
    TGuiDockerNode *window_node = window_node_alloc(0);
    tgui_docker_set_root_node(window_node);
    TGuiWindow *window = tgui_window_alloc(window_node, name, scroll, state.allocated_windows);
    ASSERT(window);
    return window->id;
}

TGuiWindowHandle tgui_split_window(TGuiWindowHandle handle, TGuiSplitDirection dir, char *name, b32 scroll) {
    TGuiWindow *window = tgui_window_get_from_handle(handle);

    TGuiDockerNode *window_node = window->parent; 
    ASSERT(window_node->type == TGUI_DOCKER_NODE_WINDOW);

    TGuiDockerNode *new_window_node = window_node_alloc(window_node->parent);
    tgui_docker_node_split(window_node, dir, TGUI_POS_FRONT, new_window_node);
    
    TGuiWindow *new_window = tgui_window_alloc(new_window_node, name, scroll, state.allocated_windows);
    ASSERT(new_window);
    return new_window->id;
}

/* ---------------------- */
/*       TGui Fuction     */
/* ---------------------- */

void tgui_initialize(void) {
    
    memset(&state, 0, sizeof(TGui));

    arena_initialize(&state.arena, 0, ARENA_TYPE_VIRTUAL);
    virtual_map_initialize(&state.registry);
    
    state.active_window = NULL;
    state.active_id = -1;

    state.window_id_generator = 0;

    state.free_windows = NULL;
    state.allocated_windows = tgui_allocated_window_node_alloc();
    clink_list_init(state.allocated_windows);

    tgui_font_initilize(&state.arena);
    tgui_docker_initialize();
}

void tgui_terminate(void) {

    tgui_serializer_write_docker_tree(docker.root, "./tgui.dat");
    
    tgui_docker_terminate();
    tgui_font_terminate();

    virtual_map_terminate(&state.registry);
    arena_terminate(&state.arena);
    memset(&state, 0, sizeof(TGui));
}

static u32 tgui_allocated_window_list_size(TGuiAllocatedWindow *list) {
    u32 result = 0;
    TGuiAllocatedWindow *allocated_window = list->next;
    while(!clink_list_end(allocated_window, list)) {
        result += 1;
        allocated_window = allocated_window->next;
    }
    return result;
}

static b32 tgui_allocated_window_equals(TGuiAllocatedWindow *allocated_window0, TGuiAllocatedWindow *allocated_window1) {

    if(allocated_window0->window.id != allocated_window1->window.id) {
        return false;
    }
    
    if(strcmp(allocated_window0->window.name, allocated_window1->window.name) != 0) {
        return false;
    }

    return true;
}

static b32 tgui_node_tree_valid(TGuiDockerNode *node, TGuiAllocatedWindow *allocated_windows) {
    UNUSED(node);
    
    if(tgui_allocated_window_list_size(state.allocated_windows) != tgui_allocated_window_list_size(allocated_windows)) {
        return false;
    }

    b32 window_found = false;

    TGuiAllocatedWindow *allocated_window = state.allocated_windows->next;
    while(!clink_list_end(allocated_window, state.allocated_windows)) {
        
        window_found = false;

        TGuiAllocatedWindow *allocated_window_test = allocated_windows->next;
        while(!clink_list_end(allocated_window_test, allocated_windows)) {

            if(tgui_allocated_window_equals(allocated_window, allocated_window_test)) {
                window_found =  true;
                break;
            }

            allocated_window_test = allocated_window_test->next;
        }
        
        if(window_found == false) {
            break;
        }

        allocated_window = allocated_window->next;
    }

    return window_found;
}

void tgui_free_allocated_windows_list(TGuiAllocatedWindow *list) {
    TGuiAllocatedWindow *allocated_window = list->next;
    while(!clink_list_end(allocated_window, list)) {
        TGuiAllocatedWindow *to_free = allocated_window;
        allocated_window = allocated_window->next;
        tgui_allocated_window_node_free(to_free);
    }
}

void tgui_try_to_load_data_file(void) {
    OsFile *file = os_file_read_entire("./tgui.dat");
    if(file) {
        TGuiAllocatedWindow allocated_windows;
        TGuiDockerNode *saved_root;
        tgui_serializer_read_docker_tree(file, &saved_root, &allocated_windows);

        if(tgui_node_tree_valid(saved_root, &allocated_windows)) {
            tgui_free_allocated_windows_list(state.allocated_windows);
            
            allocated_windows.next->prev = state.allocated_windows;
            allocated_windows.prev->next = state.allocated_windows;
            state.allocated_windows->next = allocated_windows.next;
            state.allocated_windows->prev = allocated_windows.prev;
            
            docker.root = saved_root;
        } else {
            tgui_free_allocated_windows_list(&allocated_windows);
            printf("tgui.dat file old or corrupted\n");
        }

        os_file_free(file);
    }
}

void tgui_begin(f32 dt) {
    state.dt = dt;
    tgui_docker_update();

    TGuiAllocatedWindow *allocated_window = state.allocated_windows->next;
    while(!clink_list_end(allocated_window, state.allocated_windows)) {
        
        TGuiWindow *window = &allocated_window->window;
        Rectangle window_dim = tgui_docker_get_client_rect(window->parent);
        window->dim = window_dim;
        
        window->h_scroll_bar = rect_set_invalid();
        window->v_scroll_bar = rect_set_invalid();

        allocated_window = allocated_window->next;
    }
}

static inline Rectangle calculate_total_widget_rect(TGuiWindow *window) {
    Rectangle result = window->dim;
    TGuiWidget *widget = window->widgets->next;
    while(!clink_list_end(widget, window->widgets)) {
        result = rect_union(result, calculate_widget_rect(widget));
        widget = widget->next;
    }
    return result;
}

void tgui_scroll_window_recalculate_dim(TGuiWindow *window) {
    if(!window->is_scrolling) return;
    
    b32 h_bar_added = false;
    b32 v_bar_added = false;
    Rectangle widget_rect;

    widget_rect = calculate_total_widget_rect(window);
    window->scroll_saved_rect = widget_rect;
    
    if(rect_width(widget_rect) > rect_width(window->dim)) {
        window->dim.max_y -= TGUI_SCROLL_BAR_SIZE;
        h_bar_added = true;
    }

    widget_rect = calculate_total_widget_rect(window);

    if(rect_height(widget_rect) > rect_height(window->dim)) {
        window->dim.max_x -= TGUI_SCROLL_BAR_SIZE;
        v_bar_added = true;

        if(!h_bar_added) {
            widget_rect = calculate_total_widget_rect(window);
            
            if(rect_width(widget_rect) > rect_width(window->dim)) {
                window->dim.max_y -= TGUI_SCROLL_BAR_SIZE;
                h_bar_added = true;
            }
        }
    }

    if(h_bar_added) {
        window->h_scroll_bar = window->dim;
        window->h_scroll_bar.min_y = window->dim.max_y + 1;
        window->h_scroll_bar.max_y = window->h_scroll_bar.min_y + TGUI_SCROLL_BAR_SIZE - 1;
    } else {
        window->h_scroll_offset = 0;
    }

    if(v_bar_added) {
        window->v_scroll_bar = window->dim;
        window->v_scroll_bar.min_x = window->dim.max_x + 1;
        window->v_scroll_bar.max_x = window->v_scroll_bar.min_x + TGUI_SCROLL_BAR_SIZE - 1;
    } else {
        window->v_scroll_offset = 0;
    }

}

void tgui_process_scroll_window(TGuiWindow *window, Painter *painter) {
    if(!window->is_scrolling) return;

    b32 v_scroll_valid = !rect_invalid(window->v_scroll_bar);
    b32 h_scroll_valid = !rect_invalid(window->h_scroll_bar);

    if(v_scroll_valid) {
        
        b32 mouse_over_bar = rect_point_overlaps(window->v_scroll_bar, input.mouse_x, input.mouse_y);
        if(mouse_over_bar && input.mouse_button_is_down && !input.mouse_button_was_down) {
            window->v_scroll_active = true;
        }

        if(window->v_scroll_active && !input.mouse_button_is_down) {
            window->v_scroll_active = false;
        }
        
        if(window->v_scroll_active) {
            f32 mouse_offset = (f32)(input.mouse_y - window->v_scroll_bar.min_y)/(f32)rect_height(window->v_scroll_bar);
            window->v_scroll_offset = CLAMP(mouse_offset, 0, 1);
        }
        
        u32 handle_h = (u32)(((f32)rect_height(window->dim) / (f32)rect_height(window->scroll_saved_rect)) * rect_height(window->v_scroll_bar)); 

        Rectangle handle = window->v_scroll_bar; 
        handle.min_y += window->v_scroll_offset * (rect_height(window->v_scroll_bar) - handle_h);
        handle.max_y = handle.min_y + handle_h - 1;

        painter_draw_rectangle(painter, window->v_scroll_bar, 0x333333);
        painter_draw_vline(painter, window->v_scroll_bar.min_x, window->v_scroll_bar.min_y, window->v_scroll_bar.max_y, 0x444444);
        
        painter_draw_rectangle(painter, handle, 0x555555);
        painter_draw_rectangle_outline(painter, handle, 0x888888);
    }

    if(h_scroll_valid) {

        b32 mouse_over_bar = rect_point_overlaps(window->h_scroll_bar, input.mouse_x, input.mouse_y);
        if(mouse_over_bar && input.mouse_button_is_down && !input.mouse_button_was_down) {
            window->h_scroll_active = true;
        }

        if(window->h_scroll_active && !input.mouse_button_is_down) {
            window->h_scroll_active = false;
        }
        
        if(window->h_scroll_active) {
            f32 mouse_offset = (f32)(input.mouse_x - window->h_scroll_bar.min_x)/(f32)rect_width(window->h_scroll_bar);
            window->h_scroll_offset = CLAMP(mouse_offset, 0, 1);
        }
        
        u32 handle_w = (u32)(((f32)rect_width(window->dim) / (f32)rect_width(window->scroll_saved_rect)) * rect_width(window->h_scroll_bar)); 

        Rectangle handle = window->h_scroll_bar; 
        handle.min_x += window->h_scroll_offset * (rect_width(window->h_scroll_bar) - handle_w);
        handle.max_x = handle.min_x + handle_w - 1;

        painter_draw_rectangle(painter, window->h_scroll_bar, 0x333333);
        painter_draw_hline(painter, window->h_scroll_bar.min_y, window->h_scroll_bar.min_x, window->h_scroll_bar.max_x, 0x444444);
        
        painter_draw_rectangle(painter, handle, 0x555555);
        painter_draw_rectangle_outline(painter, handle, 0x888888);
    }

    if(v_scroll_valid && h_scroll_valid) {
        Rectangle window_rect = tgui_docker_get_client_rect(window->parent);
        Rectangle inner_rect = (Rectangle){window->dim.max_x+1, window->dim.max_y+1, window_rect.max_x, window_rect.max_y};
        painter_draw_rectangle(painter, inner_rect, 0x444444);
    }
}

void tgui_end(Painter *painter) {
    
    tgui_docker_root_node_draw(painter);

    TGuiAllocatedWindow *allocated_window = state.allocated_windows->next;
    while(!clink_list_end(allocated_window, state.allocated_windows)) { 

        TGuiWindow *window = &allocated_window->window;

        tgui_scroll_window_recalculate_dim(window);
        tgui_window_process_widgets(window, painter);
        tgui_process_scroll_window(window, painter);

        tgui_window_free_widgets(window);
        
        allocated_window = allocated_window->next;
    }

    tgui_docker_draw_preview(painter);
}
