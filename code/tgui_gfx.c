#include "tgui_gfx.h"
#include "common.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include <stdio.h>

extern TGui state;

TGuiBitmap tgui_bitmap_alloc_empty(Arena *arena, u32 w, u32 h) {
    
    u64 bitmap_size = w*h*sizeof(u32);

    TGuiBitmap bitmap;
    bitmap.pixels  = arena_alloc(arena, bitmap_size, 8);
    bitmap.width   = w;
    bitmap.height  = h;
    bitmap.texture = NULL;
    
    memset(bitmap.pixels, 0, bitmap_size);

    return bitmap;
}

TGuiBitmap tgui_bitmap_copy(Arena *arena, TGuiBitmap *bitmap) {
    
    TGuiBitmap result = tgui_bitmap_alloc_empty(arena, bitmap->width, bitmap->height);
    
    u64 bitmap_size = bitmap->width*bitmap->height*sizeof(u32);
    memcpy(result.pixels, bitmap->pixels, bitmap_size);

    result.texture = bitmap->texture;
    
    return result;
}

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas) {
    arena_initialize(&texture_atlas->arena, 0, ARENA_TYPE_VIRTUAL);
    tgui_array_initialize(&texture_atlas->textures);

    texture_atlas->id = 0;

    texture_atlas->bitmap.pixels = NULL;
    texture_atlas->bitmap.height = 0;
    texture_atlas->bitmap.width = TGUI_TEXTURE_ATLAS_START_WIDTH;

    texture_atlas->current_x = 0;
    texture_atlas->current_y = 0;
}

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas) {
    tgui_array_terminate(&texture_atlas->textures);
    arena_terminate(&texture_atlas->arena);
    memset(texture_atlas, 0, sizeof(TGuiTextureAtlas));

}

void tgui_texture_atlas_add_bitmap(TGuiTextureAtlas *texture_atlas, TGuiBitmap *bitmap) {

    TGuiTexture *texture = arena_push_struct(&state.arena, TGuiTexture, 8);
    texture->bitmap = bitmap;
    texture->dim = rect_set_invalid();
    
    TGuiTextureBucket *bucket = tgui_array_push(&texture_atlas->textures);
    bucket->texture = texture;
    
    bitmap->texture = texture;
}

void texture_atlas_sort_textures_per_height(TGuiTextureAtlas *texture_atlas) {
    
    for(u32 i = 0; i < tgui_array_size(&texture_atlas->textures); ++i) {
        
        TGuiTextureBucket *bucket = tgui_array_get_ptr(&texture_atlas->textures, i);

        for(u32 j = i; j < tgui_array_size(&texture_atlas->textures); ++j) {
            
            if(i == j) continue;

            TGuiTexture *texture = bucket->texture;
            u32 h = texture->bitmap->height;

            TGuiTextureBucket *other_bucket = tgui_array_get_ptr(&texture_atlas->textures, j);
            TGuiTexture *other_texture = other_bucket->texture;

            u32 other_h = other_texture->bitmap->height;

            if(h < other_h) {
                TGuiTextureBucket temp = *bucket;
                *bucket = *other_bucket;
                *other_bucket = temp;
            }
        }
    }
}

void texture_atlas_insert(TGuiTextureAtlas *texture_atlas, TGuiTexture *texture) {
   TGuiBitmap *texture_atlas_bitmap = &texture_atlas->bitmap;
    
    TGuiBitmap *bitmap = texture->bitmap;
    ASSERT(bitmap);

    if(texture_atlas_bitmap->pixels == NULL) {

        texture_atlas->last_row_added_height = bitmap->height + TGUI_TEXTURE_ATLAS_DEFAULT_PADDING;
        texture_atlas_bitmap->height = texture_atlas->last_row_added_height;
        
        u64 texture_atlas_size = texture_atlas_bitmap->width*texture_atlas_bitmap->height*sizeof(u32);
        texture_atlas_bitmap->pixels = arena_alloc(&texture_atlas->arena, texture_atlas_size, 8);
        
        memset(texture_atlas_bitmap->pixels, 0, texture_atlas_size);
        
        texture_atlas_bitmap->pixels[0] = 0xffffffff;
        texture_atlas->current_x = 1 + TGUI_TEXTURE_ATLAS_DEFAULT_PADDING;
    }

    ASSERT(texture_atlas_bitmap->pixels);
    ASSERT(texture_atlas_bitmap->height >= bitmap->height);
    
    s32 width_left = texture_atlas_bitmap->width - texture_atlas->current_x; (void) width_left;
    if(width_left < (s32)bitmap->width) {

        printf("texture atlas was resize\n");
        
        u64 temp_arena_checkpoint = state.arena.used;
        
        TGuiBitmap old_texture_atlas_bitmap = tgui_bitmap_copy(&state.arena, &texture_atlas->bitmap);
        
        u32 new_row_height = bitmap->height + TGUI_TEXTURE_ATLAS_DEFAULT_PADDING;
        u32 new_texture_atlas_w = texture_atlas->bitmap.width;
        u32 new_texture_atlas_h = texture_atlas->bitmap.height + new_row_height;

        arena_free(&texture_atlas->arena);
        texture_atlas->bitmap = tgui_bitmap_alloc_empty(&texture_atlas->arena, new_texture_atlas_w, new_texture_atlas_h);

        Painter painter;
        Rectangle texture_atlas_rect = rect_from_wh(0, 0,texture_atlas->bitmap.width, texture_atlas->bitmap.height);
        painter_start(&painter, PAINTER_TYPE_SOFTWARE, texture_atlas_rect, 0, texture_atlas->bitmap.pixels, NULL, NULL, 0, 0);
        painter_draw_bitmap_no_alpha(&painter, 0, 0, &old_texture_atlas_bitmap);

        state.arena.used = temp_arena_checkpoint;
        
        texture_atlas->current_x = 0;
        texture_atlas->current_y += texture_atlas->last_row_added_height;

        texture_atlas->last_row_added_height = new_row_height;

    }

    texture_atlas_bitmap = &texture_atlas->bitmap;

    texture->dim = rect_from_wh(texture_atlas->current_x, texture_atlas->current_y, bitmap->width, bitmap->height);

    Painter painter;
    Rectangle texture_atlas_rect = rect_from_wh(0, 0,texture_atlas_bitmap->width, texture_atlas_bitmap->height);
    painter_start(&painter, PAINTER_TYPE_SOFTWARE, texture_atlas_rect, 0, texture_atlas_bitmap->pixels, NULL, NULL, 0, 0);
    
    painter_draw_bitmap_no_alpha(&painter, texture_atlas->current_x, texture_atlas->current_y, bitmap);
    
    texture_atlas->current_x += bitmap->width + TGUI_TEXTURE_ATLAS_DEFAULT_PADDING; 

}

void tgui_texture_atlas_generate_atlas(void) {
    TGuiTextureAtlas *texture_atlas = &state.texture_atlas;

    texture_atlas_sort_textures_per_height(texture_atlas);

    for(u32 i = 0; i < tgui_array_size(&texture_atlas->textures); ++i) {

        TGuiTextureBucket *bucket = tgui_array_get_ptr(&texture_atlas->textures, i);
        TGuiTexture *texture = bucket->texture;
        //printf("%d) bitmap height: %d\n", (i + 1), texture->bitmap->height);
        
        texture_atlas_insert(texture_atlas, texture);
    }
}
