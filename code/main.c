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

typedef struct TGuiHardwareTexture {
    u32 id;
} TGuiHardwareTexture;

typedef struct TGuiHardwareProgram {
    u32 id;
} TGuiHardwareProgram;

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


/* -------------------------------------------- */
/*         TGui Backend implementation          */
/* -------------------------------------------- */


u32 vao, vbo, ebo;
#define MAX_QUAD_PER_BATCH 1024

struct TGuiHardwareProgram *tgui_opengl_create_program(char *vert, char *frag) {
    
    OsFile *file_vert = os_file_read_entire(vert);
    OsFile *file_frag = os_file_read_entire(frag);
    
    char *v_src = file_vert->data;
    char *f_src = file_frag->data;

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
    
    TGuiHardwareProgram *program = malloc(sizeof(TGuiHardwareProgram));
    program->id = glCreateProgram();
    glAttachShader(program->id, v_shader);
    glAttachShader(program->id, f_shader);
    glLinkProgram(program->id);
    glGetProgramiv(program->id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program->id, 512, NULL, infoLog);
        printf("[SHADER PROGRAM ERROR]: %s\n", infoLog);
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    os_file_free(file_vert);
    os_file_free(file_frag);

    return program;
}

void tgui_opengl_destroy_program(struct TGuiHardwareProgram *program) {
    glDeleteProgram(program->id);
    free(program);
}

struct TGuiHardwareTexture *tgui_opengl_create_texture(u32 *data, u32 width, u32 height) {
    
    TGuiHardwareTexture *texture = (TGuiHardwareTexture *)malloc(sizeof(TGuiHardwareTexture));
    
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void tgui_opengl_destroy_texture(struct TGuiHardwareTexture *texture) {
    glDeleteTextures(1, &texture->id);
    free(texture);
}

void tgui_opengl_set_program_width_and_height(struct TGuiHardwareProgram *program, u32 width, u32 height) {
    glUseProgram(program->id);
    glUniform1i(glGetUniformLocation(program->id, "res_x"), width);
    glUniform1i(glGetUniformLocation(program->id, "res_y"), height);
}

void tgui_opengl_draw_buffers(struct TGuiHardwareProgram *program, struct TGuiHardwareTexture *texture, TGuiVertexArray *vertex_buffer, TGuiU32Array *index_buffer) {

        ASSERT(tgui_array_size(vertex_buffer) <= MAX_QUAD_PER_BATCH*4);
        ASSERT(tgui_array_size(index_buffer)  <= MAX_QUAD_PER_BATCH*6);
        
        glUseProgram(program->id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, tgui_array_size(vertex_buffer)*sizeof(TGuiVertex), tgui_array_data(vertex_buffer));
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tgui_array_size(index_buffer)*sizeof(u32), tgui_array_data(index_buffer));

        glBindVertexArray(vao);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

        glBindTexture(GL_TEXTURE_2D, texture->id);

        glDrawElements(GL_TRIANGLES, tgui_array_size(index_buffer), GL_UNSIGNED_INT, 0);

}

/* -------------------------------------------- */


int main(void) {

    os_initialize();
    
    Arena arena;
    arena_initialize(&arena, 0, ARENA_TYPE_VIRTUAL);
    
    
    s32 window_w = 800;
    s32 window_h = 600;
    struct OsWindow *window = os_window_create(&arena, "Test Window", 0, 0, window_w, window_h);
    
    os_gl_create_context(window);
    
    App app;

    TGuiGfxBackend gfx;
    gfx.create_program               = tgui_opengl_create_program;
    gfx.destroy_program              = tgui_opengl_destroy_program;
    gfx.create_texture               = tgui_opengl_create_texture;
    gfx.destroy_texture              = tgui_opengl_destroy_texture;
    gfx.set_program_width_and_height = tgui_opengl_set_program_width_and_height;
    gfx.draw_buffers                 = tgui_opengl_draw_buffers; 

    app.gfx = &gfx;

    app_initialize(&app, window_w, window_h);

    glViewport(0, 0, window_w, window_h);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);  

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
            
            glViewport(0, 0, window_w, window_h);
  
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
        
        glViewport(0, 0, window_w, window_h);
        
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        app_update(&app, seconds_per_frame);

        input->mouse_button_was_down = input->mouse_button_is_down;
        
        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time; (void)delta_time;
        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        last_time = current_time; (void)last_time; 
        

        os_gl_swap_buffers(window);
        //printf("ms: %lld\n", delta_time);
        
    }
    
    app_terminate(&app);

    os_gl_destroy_context(window);

    os_window_destroy(window);
    
    arena_terminate(&arena);
    os_terminate();

    printf("Program exit correctly!\n");

    return 0;
}
