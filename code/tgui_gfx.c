#include "tgui_gfx.h"
#include "common.h"
#include "memory.h"

extern TGui state;

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas) {
    arena_initialize(&texture_atlas->arena, 0, ARENA_TYPE_VIRTUAL);
    tgui_array_initialize(&texture_atlas->textures);

    texture_atlas->pixels = NULL;
    texture_atlas->height = 0;
    texture_atlas->width = TGUI_TEXTURE_ATLAS_START_WIDTH;

    texture_atlas->current_x = 0;
    texture_atlas->current_y = 0;
}

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas) {
    tgui_array_terminate(&texture_atlas->textures);
    arena_terminate(&texture_atlas->arena);
    memset(texture_atlas, 0, sizeof(TGuiTextureAtlas));

}

TGuiTexture *tgui_texture_atlas_create_texture(TGuiTextureAtlas *texture_atlas, u32 id, TGuiBitmap *bitmap) {
    TGuiTexture *texture = arena_push_struct(&texture_atlas->arena, TGuiTexture, 8);
    texture->id = id;
    texture->bitmap = bitmap;
    
    texture->u0 = 0;
    texture->v0 = 0;
    texture->u1 = 0;
    texture->v1 = 0;

    TGuiTextureBucket *bucket = tgui_array_push(&texture_atlas->textures);
    bucket->texture = texture;

    return texture;
}

void texture_atlas_sort_textures_per_height(TGuiTextureAtlas *texture_atlas) {
    
    for(u32 i = 0; i < tgui_array_size(&texture_atlas->textures); ++i) {
        
        TGuiTextureBucket *bucket = tgui_array_get_ptr(&texture_atlas->textures, i);
        TGuiTexture *texture = bucket->texture;

        u32 h = texture->bitmap->height;

        for(u32 j = 0; j < tgui_array_size(&texture_atlas->textures); ++j) {
            
            if(i == j) continue;

            TGuiTextureBucket *other_bucket = tgui_array_get_ptr(&texture_atlas->textures, j);
            TGuiTexture *other_texture = other_bucket->texture;

            u32 other_h = other_texture->bitmap->height;

            if(other_h > h) {
                TGuiTextureBucket temp = *bucket;
                *bucket = *other_bucket;
                *other_bucket = temp;
            }
        }
    }
}

void texture_atlas_insert(TGuiTextureAtlas *texture_atlas, TGuiTexture *texture) {
    UNUSED(texture_atlas); UNUSED(texture);
    if(texture_atlas->pixels == NULL) {
        texture_atlas->height = texture_atlas->height;
        texture_atlas->pixels = arena_alloc(&texture_atlas->arena, texture_atlas->width*texture_atlas->height*sizeof(u32), 8);
        texture_atlas->pixels[0] = 0xffffff;
        texture_atlas->current_x += 1;
    }
    
    ASSERT(texture_atlas->pixels);
    ASSERT(texture_atlas->height >= texture->bitmap->height);

}

void tgui_texture_atlas_generate_atlas(TGuiTextureAtlas *texture_atlas) {

    texture_atlas_sort_textures_per_height(texture_atlas);

    for(u32 i = 0; i < tgui_array_size(&texture_atlas->textures); ++i) {

        TGuiTextureBucket *bucket = tgui_array_get_ptr(&texture_atlas->textures, i);
        TGuiTexture *texture = bucket->texture;
        
        texture_atlas_insert(texture_atlas, texture);
    }
}
