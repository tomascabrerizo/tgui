#include "common.h"
#include "memory.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"

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
    UNUSED(app); UNUSED(dt);
    tgui_update();

    if(tgui_button(app->window3, "Click me!", 10, 10, painter)) {
        printf("click!\n");
    }

    if(tgui_button(app->window0, "Click me to!", 10, 10, painter)) {
        printf("click me to!\n");
    }

    if(tgui_button(app->window0, "Click me to! 1", 10, 50, painter)) {
        printf("click me to!\n");
    }

    if(tgui_button(app->window0, "Click me to! 2", 10, 90, painter)) {
        printf("click me to!\n");
    }

}

void app_draw(App *app, Painter *painter) {
    UNUSED(app); UNUSED(painter);
    painter_clear(painter, 0x00);
    tgui_docker_root_node_draw(painter);
}
