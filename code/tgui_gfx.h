#ifndef _TGUI_GFX_H_
#define _TGUI_GFX_H_

#include "memory.h"
#include "tgui.h"

/* ----------------------------- */
/*       TGui Texture Atlas      */
/* ----------------------------- */

typedef struct TGuiTexture {
    u32 id;
    f32 u0, u1, v0, v1;
    TGuiBitmap *bitmap;
} TGuiTexture;

typedef struct TGuiTextureBucket {
    TGuiTexture *texture;
} TGuiTextureBucket;

TGuiArray(TGuiTextureBucket, TGuiTextureArray);

#define TGUI_TEXTURE_ATLAS_START_WIDTH 1024

typedef struct TGuiTextureAtlas {
    u32 *pixels;
    
    u32 width;
    u32 height;

    u32 current_x;
    u32 current_y;

    Arena arena;
    TGuiTextureArray textures;

} TGuiTextureAtlas;

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas);

TGuiTexture *tgui_texture_atlas_create_texture(TGuiTextureAtlas *texture_atlas, u32 id, TGuiBitmap *bitmap);

void tgui_texture_atlas_generate_atlas(TGuiTextureAtlas *texture_atlas);

#endif /* _TGUI_GFX_H_ */

