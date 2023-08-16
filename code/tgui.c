#include "tgui.h"
#include "common.h"
#include "geometry.h"
#include "memory.h"
#include "painter.h"
#include "os.h"
#include "tgui_docker.h"


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
    
    struct OsFont *os_font = os_font_create(arena, "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf", 18);
    
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
    font.max_glyph_height = font.ascent - font.descent;

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
/*       TGui Fuction     */
/* ---------------------- */

void tgui_initialize(void) {
    
    memset(&state, 0, sizeof(TGui));

    arena_initialize(&state.arena, 0, ARENA_TYPE_VIRTUAL);
    virtual_map_initialize(&state.registry);
    
    tgui_font_initilize(&state.arena);
    tgui_docker_initialize();
}

void tgui_terminate(void) {
    
    tgui_docker_terminate();
    tgui_font_terminate();

    virtual_map_terminate(&state.registry);
    arena_terminate(&state.arena);
    memset(&state, 0, sizeof(TGui));
}


void tgui_update(f32 dt) {
    state.dt = dt;
    tgui_docker_update();
 
}

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
/*       TGui Widgets     */
/* ---------------------- */

static Rectangle calculate_widget_rect(TGuiDockerNode *window, s32 x, s32 y, s32 w, s32 h) {
    Rectangle window_rect = tgui_docker_get_client_rect(window);
    Rectangle rect = {
        window_rect.min_x + x,
        window_rect.min_y + y,
        window_rect.min_x + x + w - 1,
        window_rect.min_y + y + h - 1
    };
    return rect;
}

static Rectangle calculate_buttom_rect(TGuiDockerNode *window, s32 x, s32 y) {
    Rectangle window_rect = tgui_docker_get_client_rect(window);
    Rectangle button_rect = calculate_widget_rect(window, x, y, 120, 30);
    return rect_intersection(button_rect, window_rect);
}

u64 tgui_get_widget_id(char *tgui_id) {
    u64 id = tgui_hash(tgui_id, strlen(tgui_id));
    return id;
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

b32 _tgui_button(struct TGuiDockerNode *window, char *label, s32 x, s32 y, Painter *painter, char *tgui_id) {
    (void)label;
    
    if(!tgui_docker_window_is_visible(window)) {
        return false;
    }
    
    b32 result = false;

    u64 id = tgui_get_widget_id(tgui_id);
    Rectangle button_rect = calculate_buttom_rect(window, x, y); 
    
    b32 mouse_is_over = rect_point_overlaps(button_rect, input.mouse_x, input.mouse_y);
    
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

    if(mouse_is_over && !state.active) {
        state.hot = id;
    }

    if(!mouse_is_over && state.hot == id) {
        state.hot = 0;
    }
     
    /* TODO: Desing a command interface to render the complete UI independently */
    
    u32 button_color = 0xaaaaaa;
    u32 decoration_color = 0x444444;
    
    if(state.hot == id) {
        button_color = 0x999999;
    }

    if(state.active == id) {
        button_color = 0x333333;
        decoration_color = 0x999999;
    }
    
    /* TODO: All this rendering is temporal, the API should have a render independent way to give render primitives to the user */

    Rectangle saved_painter_clip = painter->clip;
    painter->clip = button_rect;
    
    painter_draw_rectangle(painter, button_rect, button_color);
    
    Rectangle label_rect = tgui_get_text_dim(0, 0, label);
    
    s32 label_x = button_rect.min_x + (rect_width(button_rect) - 1) / 2 - (rect_width(label_rect) - 1) / 2;
    s32 label_y = button_rect.min_y + (rect_height(button_rect) - 1) / 2 - (rect_height(label_rect) - 1) / 2;
    tgui_font_draw_text(painter, label_x, label_y, label,  strlen(label), decoration_color);
    painter_draw_rectangle_outline(painter, button_rect, decoration_color);

    painter->clip = saved_painter_clip;

    return result;
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

TGuiTextInput *_tgui_text_input(TGuiDockerNode *window, s32 x, s32 y, Painter *painter, char *tgui_id) {

    if(!tgui_docker_window_is_visible(window)) {
        return false;
    }

    Rectangle window_rect = tgui_docker_get_client_rect(window);
    Rectangle rect = calculate_widget_rect(window, x, y, 140, 30);
    Rectangle visible_rect = rect_intersection(rect, window_rect);
    
    TGuiKeyboard *keyboard = &input.keyboard;
    b32 mouse_is_over = rect_point_overlaps(visible_rect, input.mouse_x, input.mouse_y);

    u64 id = tgui_get_widget_id(tgui_id);
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

    u32 visible_glyphs = MAX(rect_width(visible_rect)/font.max_glyph_width - 2, 0);

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

    if(mouse_is_over && !state.active) {
        state.hot = id;
    }

    if(!mouse_is_over && state.hot == id) {
        state.hot = 0;
    }

    if(state.active == id && !mouse_is_over && input.mouse_button_was_down && !input.mouse_button_is_down) {
        state.active = 0;
    }
    
    Rectangle saved_painter_clip = painter->clip;
    painter->clip = window_rect; 
    
    u32 color = 0x333333;
    u32 decoration_color = 0x999999;
    u32 cursor_color = 0x00ff00;
    
    if(state.hot == id) {
        color = 0x444444;
    }
    
    if(state.active == id) {
        color = 0x999999;
        decoration_color = 0x333333;
    }

    painter_draw_rectangle(painter, rect, color);
    painter_draw_rectangle_outline(painter, rect, decoration_color);

    painter->clip = rect_intersection(rect, painter->clip);
    
    s32 text_x = rect.min_x + 8;
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

    return text_input;
}

