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
        painter_start(&painter, PAINTER_TYPE_SOFTWARE, texture_atlas_rect, 0, texture_atlas->bitmap.pixels, NULL);
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
    painter_start(&painter, PAINTER_TYPE_SOFTWARE, texture_atlas_rect, 0, texture_atlas_bitmap->pixels, NULL);
    
    painter_draw_bitmap_no_alpha(&painter, texture_atlas->current_x, texture_atlas->current_y, bitmap);
    
    texture_atlas->current_x += bitmap->width + TGUI_TEXTURE_ATLAS_DEFAULT_PADDING; 

}

void tgui_texture_atlas_generate_atlas(void) {
    TGuiTextureAtlas *texture_atlas = state.default_texture_atlas;

    texture_atlas_sort_textures_per_height(texture_atlas);

    for(u32 i = 0; i < tgui_array_size(&texture_atlas->textures); ++i) {

        TGuiTextureBucket *bucket = tgui_array_get_ptr(&texture_atlas->textures, i);
        TGuiTexture *texture = bucket->texture;
        //printf("%d) bitmap height: %d\n", (i + 1), texture->bitmap->height);
        
        texture_atlas_insert(texture_atlas, texture);
    }

    state.default_texture = tgui_render_state_alloc_texture(&state.render_state, texture_atlas->bitmap.pixels, texture_atlas->bitmap.width, texture_atlas->bitmap.height);
}

u32 tgui_texture_atlas_get_width(TGuiTextureAtlas *texture_atlas) {
    return texture_atlas->bitmap.width;
}

u32 tgui_texture_atlas_get_height(TGuiTextureAtlas *texture_atlas) {
    return texture_atlas->bitmap.height;
}

/* ----------------------------------- */
/*          TGui Render Buffer         */
/* ----------------------------------- */

void tgui_render_buffer_initialize(TGuiRenderBuffer *render_buffer) {

    tgui_array_initialize(&render_buffer->vertex_buffer);
    tgui_array_initialize(&render_buffer->index_buffer);

    render_buffer->program       = -1;
    render_buffer->texture       = -1;
    render_buffer->texture_atlas = NULL;

}

void tgui_render_buffer_terminate(TGuiRenderBuffer *render_buffer) {
    tgui_array_terminate(&render_buffer->vertex_buffer);
    tgui_array_terminate(&render_buffer->index_buffer);
}

void tgui_render_buffer_clear(TGuiRenderBuffer *render_buffer) {
    tgui_array_clear(&render_buffer->vertex_buffer);
    tgui_array_clear(&render_buffer->index_buffer);
}

void tgui_render_buffer_set_program(TGuiRenderBuffer *render_buffer, TGuiRenderStateProgramIndex program) {
    render_buffer->program = program;
}

void tgui_render_buffer_set_texture(TGuiRenderBuffer *render_buffer, TGuiRenderStateTextureIndex texture) {
    render_buffer->texture = texture;
}

void tgui_render_buffer_set_texture_atlas(TGuiRenderBuffer *render_buffer, TGuiTextureAtlas *texture_atlas) {
    render_buffer->texture_atlas = texture_atlas;
}

/* ----------------------------------- */
/*          TGui Render State          */
/* ----------------------------------- */

void tgui_render_state_initialize(TGuiRenderState *render_state, struct TGuiGfxBackend *gfx) {

    tgui_render_buffer_initialize(&render_state->render_buffer_tgui);
    tgui_render_buffer_initialize(&render_state->render_buffer_tgui_on_top);
    
    tgui_array_initialize(&render_state->programs);
    tgui_array_initialize(&render_state->textures);
    tgui_array_initialize(&render_state->textures_atlas);
    tgui_array_initialize(&render_state->render_buffers_custom);

    render_state->gfx = gfx;
    render_state->current_render_buffer_custom_pushed_count = 0;
}

void tgui_render_state_terminate(TGuiRenderState *render_state) {

    for(u32 i = 0; i < tgui_array_size(&render_state->programs); ++i) {
        struct TGuiHardwareProgram *program = tgui_array_get(&render_state->programs, i);
        render_state->gfx->destroy_program(program);
    }
    tgui_array_terminate(&render_state->programs);
    
    for(u32 i = 0; i < tgui_array_size(&render_state->textures); ++i) {
        struct TGuiHardwareTexture *texture = tgui_array_get(&render_state->textures, i);
        render_state->gfx->destroy_texture(texture);
    }
    tgui_array_terminate(&render_state->textures);
    
    for(u32 i = 0; i < tgui_array_size(&render_state->textures_atlas); ++i) {
        TGuiTextureAtlas *texture_atlas = tgui_array_get_ptr(&render_state->textures_atlas, i);
        tgui_texture_atlas_terminate(texture_atlas);
    }
    tgui_array_terminate(&render_state->textures_atlas);

    for(u32 i = 0; i < tgui_array_size(&render_state->render_buffers_custom); ++i) {
        TGuiRenderBuffer *render_buffer = tgui_array_get_ptr(&render_state->render_buffers_custom, i);
        tgui_render_buffer_terminate(render_buffer);
    }
    tgui_array_terminate(&render_state->render_buffers_custom);

    tgui_render_buffer_terminate(&render_state->render_buffer_tgui);
    tgui_render_buffer_terminate(&render_state->render_buffer_tgui_on_top);

}

void tgui_render_state_clear_render_buffers(TGuiRenderState *render_state) {

    tgui_render_buffer_clear(&render_state->render_buffer_tgui);
    tgui_render_buffer_clear(&render_state->render_buffer_tgui_on_top);

    for(u32 i = 0; i < tgui_array_size(&render_state->render_buffers_custom); ++i) {
        TGuiRenderBuffer *render_buffer = tgui_array_get_ptr(&render_state->render_buffers_custom, i);
        tgui_render_buffer_clear(render_buffer);
    }

    render_state->current_render_buffer_custom_pushed_count = 0;

}

TGuiRenderStateTextureIndex tgui_render_state_alloc_texture(TGuiRenderState *render_state, u32 *data, u32 width, u32 height) {
    TGuiRenderStateTextureIndex index = tgui_array_size(&render_state->textures);
    struct TGuiHardwareTexture **texture = tgui_array_push(&render_state->textures);
    *texture = render_state->gfx->create_texture(data, width, height);
    return index;
}

TGuiRenderStateProgramIndex tgui_render_state_alloc_program(TGuiRenderState *render_state, char *vert, char *frag) {
    TGuiRenderStateProgramIndex index = tgui_array_size(&render_state->programs);
    struct TGuiHardwareProgram **program = tgui_array_push(&render_state->programs);
    *program = render_state->gfx->create_program(vert, frag);
    return index;
}

TGuiTextureAtlas *tgui_render_state_alloc_texture_atlas(TGuiRenderState *render_state) {
    TGuiTextureAtlas *texture_atlas = tgui_array_push(&render_state->textures_atlas);
    tgui_texture_atlas_initialize(texture_atlas);
    return texture_atlas;
}

TGuiRenderBuffer *tgui_render_state_push_render_buffer_custom(TGuiRenderState *render_state, TGuiRenderStateProgramIndex program, TGuiRenderStateTextureIndex texture, TGuiTextureAtlas *texture_atlas) {
    TGuiRenderBuffer *render_buffer = NULL;
    
    if(render_state->current_render_buffer_custom_pushed_count >= tgui_array_size(&render_state->render_buffers_custom)) {
        render_buffer = tgui_array_push(&render_state->render_buffers_custom);
        tgui_render_buffer_initialize(render_buffer);
    } else {
        render_buffer = tgui_array_get_ptr(&render_state->render_buffers_custom, render_state->current_render_buffer_custom_pushed_count);
    }
    ++render_state->current_render_buffer_custom_pushed_count;

    tgui_render_buffer_set_program(render_buffer, program);
    tgui_render_buffer_set_texture(render_buffer, texture);
    tgui_render_buffer_set_texture_atlas(render_buffer, texture_atlas);

    ASSERT(render_buffer);
    return render_buffer;
}

struct TGuiHardwareProgram *tgui_render_state_get_program(TGuiRenderState *render_state, TGuiRenderStateProgramIndex index) {
    ASSERT((index >= 0) && (index < (s32)tgui_array_size(&render_state->programs)));
    return tgui_array_get(&render_state->programs, index);
}

struct TGuiHardwareTexture *tgui_render_state_get_texture(TGuiRenderState *render_state, TGuiRenderStateTextureIndex index) {
    ASSERT((index >= 0) && (index < (s32)tgui_array_size(&render_state->textures)));
    return tgui_array_get(&render_state->textures, index);
}

void tgui_render_state_update_programs_width_and_height(TGuiRenderState *render_state, u32 width, u32 height) {
    for(u32 i = 0; i < tgui_array_size(&render_state->programs); ++i) {
        struct TGuiHardwareProgram *program = tgui_array_get(&render_state->programs, i);
        render_state->gfx->set_program_width_and_height(program, width, height);
    }

}

void tgui_render_buffer_draw(TGuiRenderState *render_state, TGuiRenderBuffer *render_buffer) {
    struct TGuiHardwareProgram *program = tgui_render_state_get_program(render_state, render_buffer->program);
    struct TGuiHardwareTexture *texture = tgui_render_state_get_texture(render_state, render_buffer->texture);
    render_state->gfx->draw_buffers(program, texture, &render_buffer->vertex_buffer, &render_buffer->index_buffer);
}

void tgui_render_state_draw_buffers(TGuiRenderState *render_state) {

    ASSERT(render_state->current_render_buffer_custom_pushed_count <= tgui_array_size(&render_state->render_buffers_custom));

    for(u32 i = 0; i < render_state->current_render_buffer_custom_pushed_count; ++i) {

        TGuiRenderBuffer *render_buffer = tgui_array_get_ptr(&render_state->render_buffers_custom, i);

        tgui_render_buffer_draw(render_state, render_buffer);
    }
    
    tgui_render_buffer_draw(render_state, &render_state->render_buffer_tgui);

    tgui_render_buffer_draw(render_state, &render_state->render_buffer_tgui_on_top);
}
