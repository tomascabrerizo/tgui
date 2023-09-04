#include "common.h"
#include "geometry.h"
#include "os.h"
#include "memory.h"
#include "os_gl.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"
#include "tgui_serializer.h"
#include "tgui_gfx.h"

#define HARWARE_RENDERING 1

#include "os.c"
#include "geometry.c"
#include "memory.c"
#include "tgui.c"
#include "tgui_docker.c"
#include "tgui_serializer.c"
#include "tgui_gfx.c"

#include <GL/gl.h>
#include <GL/glext.h>

#include "painter.c"

#include "app.c"
#include <stdio.h>

struct OsBackbuffer *realloc_backbuffer(struct OsBackbuffer *backbuffer, struct OsWindow *window, s32 w, s32 h) {
    
    static Arena arena;
    static b32 arena_is_initialize = false;
    if(!arena_is_initialize) {
        arena_initialize(&arena, 0, ARENA_TYPE_VIRTUAL);
        arena_is_initialize = true;
    }
    
    if(!backbuffer) {
        u32 *pixels = (u32 *)arena_alloc(&arena, w*h*sizeof(u32), 8);
        backbuffer = os_backbuffer_create(&arena, window, pixels, w, h, sizeof(u32));
    } else {
        os_backbuffer_destroy(backbuffer);
        arena_free(&arena);
        u32 *pixels = (u32 *)arena_alloc(&arena, w*h*sizeof(u32), 8);
        backbuffer = os_backbuffer_create(&arena, window, pixels, w, h, sizeof(u32));
    }

    return backbuffer;
}

unsigned int program_create(char *v_src, char *f_src) {
    int success;
    static char infoLog[512];

    unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, (const char **)&v_src, NULL);
    glCompileShader(v_shader);
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(v_shader, 512, NULL, infoLog);
        printf("[VERTEX SHADER ERROR]: %s\n", infoLog);
    }

    unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, (const char **)&f_src, NULL);
    glCompileShader(f_shader);
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(f_shader, 512, NULL, infoLog);
        printf("[FRAGMENT SHADER ERROR]: %s\n", infoLog);
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("[SHADER PROGRAM ERROR]: %s\n", infoLog);
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    return program;
}

int main(void) {

    os_initialize();
    
    Arena arena;
    arena_initialize(&arena, 0, ARENA_TYPE_VIRTUAL);
    
    
    s32 window_w = 800;
    s32 window_h = 600;
    struct OsWindow *window = os_window_create(&arena, "Test Window", 0, 0, window_w, window_h);

    App app;
    app_initialize(&app, window_w, window_h);

#if HARWARE_RENDERING
    os_gl_create_context(window);

    glViewport(0, 0, window_w, window_h);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_MULTISAMPLE);  

    TGuiVertexArray vertex_array;
    TGuiU32Array  index_array;

    tgui_array_initialize(&vertex_array);
    tgui_array_initialize(&index_array);

    #define MAX_QUAD_PER_BATCH 1024

    OsFile *triangle_frag = os_file_read_entire("./shaders/triangle.frag");
    OsFile *triangle_vert = os_file_read_entire("./shaders/triangle.vert");
    u32 triangle_program = program_create(triangle_vert->data, triangle_frag->data);

    OsFile *frag = os_file_read_entire("./shaders/quad.frag");
    OsFile *vert = os_file_read_entire("./shaders/quad.vert");
    
    u32 program = program_create(vert->data, frag->data);
    glUseProgram(program);

    u32 vao, vbo, ebo;
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUAD_PER_BATCH*(sizeof(TGuiVertex) * 4), 0, GL_DYNAMIC_DRAW); 
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), OFFSET_OF(TGuiVertex, x)); 

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), OFFSET_OF(TGuiVertex, u)); 

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TGuiVertex), OFFSET_OF(TGuiVertex, r)); 

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUAD_PER_BATCH*(sizeof(u32) * 6), 0, GL_DYNAMIC_DRAW); 
    
    TGuiTextureAtlas *texture_atlas = tgui_get_texture_atlas();
    glGenTextures(1, &texture_atlas->id);
    glBindTexture(GL_TEXTURE_2D, texture_atlas->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
            texture_atlas->bitmap.width, texture_atlas->bitmap.height, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, texture_atlas->bitmap.pixels);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas->id);

    /* --------------------------------------------- */
    /* NOTE: Custom FrameBuffer */
    
    u32 fbo_texture;
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    u32 rbo_depth;
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1024, 1024);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    u32 fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("Cannot create frame buffer\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* --------------------------------------------- */


#else
    struct OsBackbuffer *backbuffer = realloc_backbuffer(0, window, window_w, window_h);
#endif

    u64 miliseconds_per_frame = 16;
    f32 seconds_per_frame = (f32)miliseconds_per_frame / 1000.0f; (void)seconds_per_frame;
    u64 last_time = os_get_ticks();
     

    TGuiInput *input = tgui_get_input();
    
    while(!os_window_should_close(window)) {
        
        os_window_poll_events(window);
        
        if(os_window_resize(window)) {

            window_w = os_window_width(window);
            window_h = os_window_height(window);
            
            if(window_w <= 1) {
                u32 break_here = 0;
                (void)break_here;
            }
#if HARWARE_RENDERING
            glViewport(0, 0, window_w, window_h);
#else
            backbuffer = realloc_backbuffer(backbuffer, window, window_w, window_h);
#endif
            input->window_resize = true;
            input->resize_w = window_w;
            input->resize_h = window_h;
        }

        os_window_get_mouse_position(window, &input->mouse_x, &input->mouse_y);
        os_window_get_mouse_lbutton_state(window, &input->mouse_button_is_down);
        os_window_get_text_input(window, input->text, &input->text_size, TGUI_MAX_TEXT_SIZE);
        os_window_get_keyboard_input(window, &input->keyboard);


        switch (tgui_get_cursor_state()) { 
        
        case TGUI_CURSOR_ARROW:   { os_set_cursor(window, OS_CURSOR_ARROW);   } break;
        case TGUI_CURSOR_HAND:    { os_set_cursor(window, OS_CURSOR_HAND);    } break;
        case TGUI_CURSOR_V_ARROW: { os_set_cursor(window, OS_CURSOR_V_ARROW); } break;
        case TGUI_CURSOR_H_ARROW: { os_set_cursor(window, OS_CURSOR_H_ARROW); } break;

        }
        
        Painter painter;
#if HARWARE_RENDERING
        glViewport(0, 0, window_w, window_h);
        
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //printf("vertex array size: %lld, capacity: %lld, arena used: %lld, arena size: %lld\n", tgui_array_size(&vertex_array), vertex_array.type_array.capacity, vertex_array.type_array.arena.used, vertex_array.type_array.arena.size);
        //printf("index array size: %lld, capacity: %lld, arena used: %lld, arena size: %lld\n", tgui_array_size(&index_array), index_array.type_array.capacity, index_array.type_array.arena.used, index_array.type_array.arena.size);
        painter_start(&painter, PAINTER_TYPE_HARDWARE, (Rectangle){0, 0, window_w-1, window_h-1}, 0, NULL, &vertex_array, &index_array, texture_atlas->bitmap.width, texture_atlas->bitmap.height);
#else
        painter_start(&painter, PAINTER_TYPE_SOFTWARE, (Rectangle){0, 0, window_w-1, window_h-1}, 0, (u32 *)backbuffer->data, NULL, NULL, 0, 0);
#endif
        app_update(&app, seconds_per_frame, &painter);

#if HARWARE_RENDERING
        
        /* ------------------------------------------ */
        /* Render frame buffer */
        
        TGuiWindow *w = tgui_window_get_from_handle(app.window1);

        if(tgui_window_update_widget(w)) {


            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, 1024, 1024);
            glUseProgram(triangle_program);

            TGuiVertex tri_vertices[3] = {
                {-0.5f, -0.5f,   0, 0,  1,0,0}, 
                { 0.5f, -0.5f,   0, 1,  0,1,0}, 
                { 0.0f,  0.5f,   1, 1,  0,0,1}, 
            };
            
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 3*sizeof(TGuiVertex), tri_vertices);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, window_w, window_h);

            TGuiVertex quad_vertices[6] = {
                {w->dim.min_x  , w->dim.min_y  ,   0, 1,  1,1,1}, 
                {w->dim.min_x  , w->dim.max_y+1,   0, 0,  1,1,1}, 
                {w->dim.max_x+1, w->dim.max_y+1,   1, 0,  1,1,1}, 
                
                {w->dim.max_x+1, w->dim.max_y+1,   1, 0,  1,1,1}, 
                {w->dim.max_x+1, w->dim.min_y  ,   1, 1,  1,1,1}, 
                {w->dim.min_x  , w->dim.min_y  ,   0, 1,  1,1,1} 
            };

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 6*sizeof(TGuiVertex), quad_vertices);
            
            glUseProgram(program);
            glBindTexture(GL_TEXTURE_2D, fbo_texture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        /* ------------------------------------------ */


    ASSERT(tgui_array_size(&vertex_array) <= MAX_QUAD_PER_BATCH*4);
    ASSERT(tgui_array_size(&index_array)  <= MAX_QUAD_PER_BATCH*6);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, tgui_array_size(&vertex_array)*sizeof(TGuiVertex), tgui_array_data(&vertex_array));
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tgui_array_size(&index_array)*sizeof(u32), tgui_array_data(&index_array));

    glBindVertexArray(vao);
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "res_x"), window_w);
    glUniform1i(glGetUniformLocation(program, "res_y"), window_h);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBindTexture(GL_TEXTURE_2D, texture_atlas->id);

    glDrawElements(GL_TRIANGLES, tgui_array_size(&index_array), GL_UNSIGNED_INT, 0);


#endif

        input->mouse_button_was_down = input->mouse_button_is_down;
        
        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time; (void)delta_time;
        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        last_time = current_time; (void)last_time; 
        

#if HARWARE_RENDERING
        os_gl_swap_buffers(window);
#else
        os_backbuffer_swap(window, backbuffer);
#endif
        //printf("ms: %lld\n", delta_time);
        
    }
    
    app_terminate(&app);

#if HARWARE_RENDERING
    os_gl_destroy_context(window);
#else
    os_backbuffer_destroy(backbuffer);
#endif

    os_window_destroy(window);
    
    arena_terminate(&arena);
    os_terminate();

    printf("Program exit correctly!\n");

    return 0;
}
