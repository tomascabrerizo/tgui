#ifndef _TGUI_GFX_H_
#define _TGUI_GFX_H_

#include "geometry.h"
#include "memory.h"

/* ----------------------- */
/*       TGui Bitmap       */
/* ----------------------- */

typedef struct TGuiBitmap {
    u32 *pixels;
    u32 width, height;
    struct TGuiTexture *texture;
} TGuiBitmap;

TGuiBitmap tgui_bitmap_alloc_empty(Arena *arena, u32 w, u32 h);

TGuiBitmap tgui_bitmap_copy(Arena *arena, TGuiBitmap *bitmap);

/* ------------------------ */
/*       TGui Texture       */
/* ------------------------ */

typedef struct TGuiTexture {
    Rectangle dim;
    TGuiBitmap *bitmap;
} TGuiTexture;

typedef struct TGuiTextureBucket {
    TGuiTexture *texture;
} TGuiTextureBucket;

TGuiArray(TGuiTextureBucket, TGuiTextureArray);

#define TGUI_TEXTURE_ATLAS_START_WIDTH 1024
#define TGUI_TEXTURE_ATLAS_DEFAULT_PADDING 4

/* ----------------------------- */
/*       TGui Texture Atlas      */
/* ----------------------------- */

typedef struct TGuiTextureAtlas {
    u32 id;
    
    TGuiBitmap bitmap;

    u32 current_x;
    u32 current_y;

    Arena arena;
    TGuiTextureArray textures;
    
    u32 last_row_added_height;

} TGuiTextureAtlas;

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_add_bitmap(TGuiTextureAtlas *texture_atlas, TGuiBitmap *bitmap);

void tgui_texture_atlas_generate_atlas(void);

#endif /* _TGUI_GFX_H_ */

