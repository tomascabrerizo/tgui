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
    
    TGuiBitmap bitmap;

    u32 current_x;
    u32 current_y;
    u32 last_row_added_height;

    Arena arena;
    TGuiTextureArray textures;
    
} TGuiTextureAtlas;

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_add_bitmap(TGuiTextureAtlas *texture_atlas, TGuiBitmap *bitmap);

void tgui_texture_atlas_generate_atlas(void);

u32 tgui_texture_atlas_get_width(TGuiTextureAtlas *texture_atlas);

u32 tgui_texture_atlas_get_height(TGuiTextureAtlas *texture_atlas);

/* ----------------------------- */
/*          TGui Vertext         */
/* ----------------------------- */

typedef struct TGuiVertex {
    float x, y;
    float u, v;
    float r, g, b;
} TGuiVertex;

TGuiArray(TGuiVertex, TGuiVertexArray);
TGuiArray(u32, TGuiU32Array);

/* ----------------------------------- */
/*          TGui Render Buffer         */
/* ----------------------------------- */

struct TGuiHardwareTexture;
struct TGuiHardwareProgram;

typedef struct TGuiRenderBuffer {

    struct TGuiHardwareProgram *program;
    struct TGuiHardwareTexture *texture;
    
    TGuiVertexArray vertex_buffer;
    TGuiU32Array    index_buffer;
    
    TGuiTextureAtlas *texture_atlas;

} TGuiRenderBuffer;

void tgui_render_buffer_initialize(TGuiRenderBuffer *render_buffer, struct TGuiHardwareProgram *program, struct TGuiHardwareTexture *texture, TGuiTextureAtlas *texture_atlas);

void tgui_render_buffer_terminate(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_clear(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_set_program(TGuiRenderBuffer *render_buffer, struct TGuiHardwareProgram *program);

void tgui_render_buffer_set_texture(TGuiRenderBuffer *render_buffer, struct TGuiHardwareTexture *texture);

/* -------------------------------------------- */
/*         TGui Platform specifyc backend       */
/* -------------------------------------------- */

/* NOTE: This functions must be implemented by the backend */

typedef struct TGuiHardwareProgram *(*TGuiGfxCreateProgram) (char *vert, char *frag);
typedef void (*TGuiGfxDestroyProgram) (struct TGuiHardwareProgram *program);

typedef struct TGuiHardwareTexture *(*TGuiGfxCreateTexture) (u32 *data, u32 width, u32 height);
typedef void (*TGuiGfxDestroyTexture) (struct TGuiHardwareTexture *texture);

typedef void (*TGuiGfxSetProgramWidthAndHeight) (struct TGuiHardwareProgram *program, u32 width, u32 height);

typedef void (*TGuiGfxDrawBuffers) (struct TGuiHardwareProgram *program, struct TGuiHardwareTexture *texutre, TGuiVertexArray *vertex_buffer, TGuiU32Array *index_buffer);


typedef struct TGuiGfxBackend {
    
    TGuiGfxCreateProgram  create_program; 
    TGuiGfxDestroyProgram destroy_program; 

    TGuiGfxCreateTexture  create_texture; 
    TGuiGfxDestroyTexture destroy_texture;
    
    TGuiGfxSetProgramWidthAndHeight set_program_width_and_height;

    TGuiGfxDrawBuffers draw_buffers;

} TGuiGfxBackend;


#endif /* _TGUI_GFX_H_ */

