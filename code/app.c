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

    app->window0 = tgui_docker_create_root_window();
    app->window1 = tgui_docker_split_window(app->window0, TGUI_SPLIT_DIR_VERTICAL);
    app->window2 = tgui_docker_split_window(app->window1, TGUI_SPLIT_DIR_VERTICAL);
    app->window3 = tgui_docker_split_window(app->window1, TGUI_SPLIT_DIR_HORIZONTAL);

}

void app_terminate(App *app) {
    UNUSED(app);

    tgui_terminate();
}

void app_update(App *app, f32 dt) {
    UNUSED(app); UNUSED(dt);
    tgui_update();
}

void app_draw(App *app, Painter *painter) {
    UNUSED(app); UNUSED(painter);
    painter_clear(painter, 0x00);
    tgui_docker_root_node_draw(painter);
}
