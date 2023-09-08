#ifndef _TGUI_GFX_H_
#define _TGUI_GFX_H_

#include "tgui_geometry.h"
#include "tgui_memory.h"

/* ----------------------- */
/*       TGui Bitmap       */
/* ----------------------- */

typedef struct TGuiBitmap {
    tgui_u32 *pixels;
    tgui_u32 width, height;
    struct TGuiTexture *texture;
} TGuiBitmap;

TGuiBitmap tgui_bitmap_alloc_empty(TGuiArena *arena, tgui_u32 w, tgui_u32 h);

TGuiBitmap tgui_bitmap_copy(TGuiArena *arena, TGuiBitmap *bitmap);

/* ------------------------ */
/*       TGui Texture       */
/* ------------------------ */

typedef struct TGuiTexture {
    TGuiRectangle dim;
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

    tgui_u32 current_x;
    tgui_u32 current_y;
    tgui_u32 last_row_added_height;

    TGuiArena arena;
    TGuiTextureArray textures;
    
} TGuiTextureAtlas;

void tgui_texture_atlas_initialize(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_terminate(TGuiTextureAtlas *texture_atlas);

void tgui_texture_atlas_add_bitmap(TGuiTextureAtlas *texture_atlas, TGuiBitmap *bitmap);

void tgui_texture_atlas_generate_atlas(void);

tgui_u32 tgui_texture_atlas_get_width(TGuiTextureAtlas *texture_atlas);

tgui_u32 tgui_texture_atlas_get_height(TGuiTextureAtlas *texture_atlas);

/* ----------------------------- */
/*          TGui Vertext         */
/* ----------------------------- */

typedef struct TGuiVertex {
    float x, y;
    float u, v;
    float r, g, b;
} TGuiVertex;

TGuiArray(TGuiVertex, TGuiVertexArray);
TGuiArray(tgui_u32, TGuiU32Array);

/* ----------------------------------- */
/*          TGui Render Buffer         */
/* ----------------------------------- */

struct TGuiRenderState;

typedef struct TGuiRenderBuffer {

    void *program;
    void *texture;
    TGuiTextureAtlas *texture_atlas;
    
    TGuiVertexArray vertex_buffer;
    TGuiU32Array    index_buffer;
    

} TGuiRenderBuffer;

void tgui_render_buffer_initialize(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_terminate(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_clear(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_set_program(TGuiRenderBuffer *render_buffer, void *program);

void tgui_render_buffer_set_texture(TGuiRenderBuffer *render_buffer, void *texture);

void tgui_render_buffer_set_texture_atlas(TGuiRenderBuffer *render_buffer, TGuiTextureAtlas *texture_atlas);

void tgui_render_buffer_draw(struct TGuiRenderState *render_state, TGuiRenderBuffer *render_buffer);

/* ----------------------------------- */
/*          TGui Render State          */
/* ----------------------------------- */

TGuiArray(TGuiRenderBuffer, TGuiRenderBufferArray);

typedef struct TGuiRenderState {
    
    struct TGuiGfxBackend *gfx;

    TGuiRenderBuffer      render_buffer_tgui;
    TGuiRenderBuffer      render_buffer_tgui_on_top;
    
    TGuiRenderBufferArray render_buffers_custom;
    tgui_u32 current_render_buffer_custom_pushed_count;

} TGuiRenderState;

void tgui_render_state_initialize(TGuiRenderState *render_state, struct TGuiGfxBackend *gfx);

void tgui_render_state_terminate(TGuiRenderState *render_state);

TGuiRenderBuffer *tgui_render_state_push_render_buffer_custom(TGuiRenderState *render_state, void *program, void *texture, TGuiTextureAtlas *texture_atlas);

void tgui_render_state_clear_render_buffers(TGuiRenderState *render_state);

void tgui_render_state_draw_buffers(TGuiRenderState *render_state);


/* -------------------------------------------- */
/*         TGui Platform specifyc backend       */
/* -------------------------------------------- */

/* NOTE: This functions must be implemented by the backend */

typedef void *(*TGuiGfxCreateProgram) (char *vert, char *frag);
typedef void (*TGuiGfxDestroyProgram) (void *program);

typedef void *(*TGuiGfxCreateTexture) (tgui_u32 *data, tgui_u32 width, tgui_u32 height);
typedef void (*TGuiGfxDestroyTexture) (void *texture);

typedef void (*TGuiGfxSetProgramWidthAndHeight) (void *program, tgui_u32 width, tgui_u32 height);

typedef void (*TGuiGfxDrawBuffers) (void *program, void *texutre, TGuiVertexArray *vertex_buffer, TGuiU32Array *index_buffer);


typedef struct TGuiGfxBackend {
    
    TGuiGfxCreateProgram  create_program; 
    TGuiGfxDestroyProgram destroy_program; 

    TGuiGfxCreateTexture  create_texture; 
    TGuiGfxDestroyTexture destroy_texture;
    
    TGuiGfxSetProgramWidthAndHeight set_program_width_and_height;

    TGuiGfxDrawBuffers draw_buffers;

} TGuiGfxBackend;


#endif /* _TGUI_GFX_H_ */

