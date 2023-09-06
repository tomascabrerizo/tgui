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

struct TGuiRenderState;
typedef s32 TGuiRenderStateTextureIndex;
typedef s32 TGuiRenderStateProgramIndex;

typedef struct TGuiRenderBuffer {

    TGuiRenderStateProgramIndex program;
    TGuiRenderStateTextureIndex texture;
    TGuiTextureAtlas *texture_atlas;
    
    TGuiVertexArray vertex_buffer;
    TGuiU32Array    index_buffer;
    

} TGuiRenderBuffer;


TGuiArray(struct TGuiHardwareTexture *, TGuiHardwareTextureArray);
TGuiArray(struct TGuiHardwareProgram *, TGuiHardwareProgramArray);
TGuiArray(TGuiTextureAtlas, TGuiTextureAtlasArray);
TGuiArray(TGuiRenderBuffer, TGuiRenderBufferArray);

void tgui_render_buffer_initialize(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_terminate(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_clear(TGuiRenderBuffer *render_buffer);

void tgui_render_buffer_set_program(TGuiRenderBuffer *render_buffer, TGuiRenderStateProgramIndex program);

void tgui_render_buffer_set_texture(TGuiRenderBuffer *render_buffer, TGuiRenderStateTextureIndex texture);

void tgui_render_buffer_set_texture_atlas(TGuiRenderBuffer *render_buffer, TGuiTextureAtlas *texture_atlas);

void tgui_render_buffer_draw(struct TGuiRenderState *render_state, TGuiRenderBuffer *render_buffer);

/* ----------------------------------- */
/*          TGui Render State          */
/* ----------------------------------- */

typedef struct TGuiRenderState {
    
    struct TGuiGfxBackend *gfx;
    
    TGuiHardwareProgramArray programs;
    TGuiHardwareTextureArray textures;
    TGuiTextureAtlasArray    textures_atlas;

    TGuiRenderBuffer      render_buffer_tgui;
    TGuiRenderBuffer      render_buffer_tgui_on_top;
    
    TGuiRenderBufferArray render_buffers_custom;
    u32 current_render_buffer_custom_pushed_count;

} TGuiRenderState;

void tgui_render_state_initialize(TGuiRenderState *render_state, struct TGuiGfxBackend *gfx);

void tgui_render_state_terminate(TGuiRenderState *render_state);

TGuiRenderStateTextureIndex tgui_render_state_alloc_texture(TGuiRenderState *render_state, u32 *data, u32 width, u32 height);

TGuiRenderStateProgramIndex tgui_render_state_alloc_program(TGuiRenderState *render_state, char *vert, char *frag);

TGuiTextureAtlas *tgui_render_state_alloc_texture_atlas(TGuiRenderState *render_state);

TGuiRenderBuffer *tgui_render_state_push_render_buffer_custom(TGuiRenderState *render_state, TGuiRenderStateProgramIndex program, TGuiRenderStateTextureIndex texture, TGuiTextureAtlas *texture_atlas);

void tgui_render_state_clear_render_buffers(TGuiRenderState *render_state);

struct TGuiHardwareProgram *tgui_render_state_get_program(TGuiRenderState *render_state, TGuiRenderStateProgramIndex index);

struct TGuiHardwareTexture *tgui_render_state_get_texture(TGuiRenderState *render_state, TGuiRenderStateTextureIndex index);

void tgui_render_state_update_programs_width_and_height(TGuiRenderState *render_state, u32 width, u32 height);

void tgui_render_state_draw_buffers(TGuiRenderState *render_state);


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

