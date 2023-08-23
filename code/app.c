#include "common.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"
#include <stdio.h>

typedef struct App {
    TGuiWindow *window0;
    TGuiWindow *window1;
    TGuiWindow *window2;
    TGuiWindow *window3;

    u32 color0;
    u32 color1;

} App;

void app_initialize(App *app) {

    tgui_initialize();

    app->window0 = tgui_create_root_window("Window 0", false);
    app->window1 = tgui_split_window(app->window0, TGUI_SPLIT_DIR_HORIZONTAL, "Window 1", true);
    app->window2 = tgui_split_window(app->window0, TGUI_SPLIT_DIR_VERTICAL,   "Window 2", true);
    app->window3 = tgui_split_window(app->window1, TGUI_SPLIT_DIR_HORIZONTAL, "Window 3", false);
}

void app_terminate(App *app) {
    UNUSED(app);
    tgui_terminate();
}

void app_update(App *app, f32 dt, Painter *painter) {
    
    tgui_begin(dt);

    if(tgui_button(app->window0, "button", 10, 10)) {
        printf("click! 0\n");
    }

    if(tgui_button(app->window0, "button", 10, 60)) {
        printf("click! 1\n");
    }
    if(tgui_button(app->window3, "button", 10, 10)) {
        printf("click! 2\n");
    }
    if(tgui_button(app->window3, "button", 160, 10)) {
        printf("click! 3\n");
    }
    if(tgui_button(app->window3, "button", 310, 10)) {
        printf("click! 4\n");
    }
    
    tgui_text_input(app->window2, 10, 10);
    tgui_text_input(app->window2, 180, 10);
    
    tgui_color_picker(app->window2, 10, 60, 256, 256, &app->color0);
    tgui_color_picker(app->window1, 10, 10, 300, 100, &app->color1);

    tgui_end(painter);

}
