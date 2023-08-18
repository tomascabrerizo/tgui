#include "common.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"
#include <stdio.h>

typedef struct App {
    TGuiDockerNode *window0;
    TGuiDockerNode *window1;
    TGuiDockerNode *window2;
    TGuiDockerNode *window3;

} App;

void app_initialize(App *app) {

    tgui_initialize();

    app->window0 = tgui_docker_create_root_window("Window 0");
    app->window1 = tgui_docker_split_window(app->window0, TGUI_SPLIT_DIR_VERTICAL,   "Window 1");
    app->window2 = tgui_docker_split_window(app->window1, TGUI_SPLIT_DIR_VERTICAL,   "Window 2");
    app->window3 = tgui_docker_split_window(app->window1, TGUI_SPLIT_DIR_HORIZONTAL, "Window 3");

}

void app_terminate(App *app) {
    UNUSED(app);
    tgui_terminate();
}

void app_update(App *app, f32 dt, Painter *painter) {
    
    tgui_update(dt);
    tgui_docker_root_node_draw(painter);

    if(tgui_button(app->window0, "button", 10, 10, painter)) {
        printf("click! 0\n");
    }

    if(tgui_button(app->window0, "button", 10, 60, painter)) {
        printf("click! 1\n");
    }

    if(tgui_button(app->window3, "button", 10, 10, painter)) {
        printf("click! 2\n");
    }
    if(tgui_button(app->window3, "button", 160, 10, painter)) {
        printf("click! 3\n");
    }
    if(tgui_button(app->window3, "button", 320, 10, painter)) {
        printf("click! 4\n");
    }
    
    tgui_text_input(app->window2, 10, 10, painter);
    tgui_text_input(app->window2, 180, 10, painter);

    _tgui_drop_down_menu_begin(app->window2, 10, 60, painter, TGUI_ID);
    if(_tgui_drop_down_menu_item("item 1", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 2", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 3", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 4", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 5", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 6", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 7", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 8", painter)) {
    }
    if(_tgui_drop_down_menu_item("item 9", painter)) {
    }
    _tgui_drop_down_menu_end(painter);

#if 0
    _tgui_drop_down_menu_begin(app->window1, 10, 60, painter, TGUI_ID);
    if(_tgui_drop_down_menu_item("dimond", painter)) {
    }
    if(_tgui_drop_down_menu_item("restone", painter)) {
    }
    if(_tgui_drop_down_menu_item("dirt", painter)) {
    }
    if(_tgui_drop_down_menu_item("grass", painter)) {
    }
    _tgui_drop_down_menu_end(painter);
#endif

    _tgui_color_picker(app->window2, 180, 60, 256, 256, painter, TGUI_ID);
    _tgui_color_picker(app->window1, 10, 10, 128, 128, painter, TGUI_ID);

}
