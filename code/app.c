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

    if(tgui_button(app->window0, "button", 10, 50, painter)) {
        printf("click! 1\n");
    }

    if(tgui_button(app->window0, "button", 10, 90, painter)) {
        printf("click! 2\n");
    }

    if(tgui_button(app->window3, "button", 10, 10, painter)) {
        printf("click! 3\n");
    }
    
    tgui_text_input(app->window2, 10, 10, painter);
    tgui_text_input(app->window1, 10, 10, painter);

    if(tgui_button(app->window2, "button", 10, 110, painter)) {
        printf("click! 3\n");
    }

    _tgui_drop_down_menu_begin(app->window2, 10, 60, painter, TGUI_ID);
    if(_tgui_drop_down_menu_item("item1", painter)) {
    }
    if(_tgui_drop_down_menu_item("item2", painter)) {
    }
    if(_tgui_drop_down_menu_item("item3", painter)) {
    }
    if(_tgui_drop_down_menu_item("item4", painter)) {
    }
    _tgui_drop_down_menu_end(painter);

}
