#include "common.h"
#include "memory.h"
#include "os.h"
#include "painter.h"
#include "tgui.h"
#include "tgui_docker.h"
#include "tgui_serializer.h"
#include <stdio.h>

typedef struct App {
    TGuiWindowHandle window0;
    TGuiWindowHandle window1;
    TGuiWindowHandle window2;
    TGuiWindowHandle window3;
    TGuiWindowHandle window4;

    u32 color0;
    u32 color1;

} App;

extern TGuiDocker docker;

#define APP_ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))

void add_docker_nodes(TGuiDockerNode *node) {
   
    switch (node->type) {
    
    case TGUI_DOCKER_NODE_ROOT:   { 
        _tgui_tree_view_root_node_begin("root node", node);
        TGuiDockerNode *child = node->childs->next;
        while(!clink_list_end(child, node->childs)) {
            add_docker_nodes(child);
            child = child->next;
        }
        _tgui_tree_view_root_node_end();
    } break;
    case TGUI_DOCKER_NODE_SPLIT:  {
        _tgui_tree_view_node("split node", node);
    } break;
    case TGUI_DOCKER_NODE_WINDOW: {
        _tgui_tree_view_root_node_begin("window node", node);
        TGuiWindow *window = node->windows->next; 
        while(!clink_list_end(window, node->windows)) {
            _tgui_tree_view_node(window->name, window);
            window = window->next;
        }

        _tgui_tree_view_root_node_end();
    } break;
    
    }
}

void app_initialize(App *app) {
    UNUSED(app);
    tgui_initialize();

    app->window0 = tgui_create_root_window("Window 0", false);
    app->window1 = tgui_split_window(app->window0, TGUI_SPLIT_DIR_HORIZONTAL, "Window 1", true);
    app->window2 = tgui_split_window(app->window0, TGUI_SPLIT_DIR_VERTICAL,   "Window 2", true);
    app->window3 = tgui_split_window(app->window1, TGUI_SPLIT_DIR_HORIZONTAL, "Window 3", false);
    app->window4 = tgui_split_window(app->window3, TGUI_SPLIT_DIR_VERTICAL,   "Window 4", true);

    tgui_try_to_load_data_file();
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
    
    tgui_color_picker(app->window1, 10, 10, 300, 100, &app->color1);
    
    void *user_data = NULL;

    _tgui_tree_view_begin(app->window4, TGUI_ID);
        add_docker_nodes(docker.root);
    _tgui_tree_view_end(&user_data);

    if(tgui_button(app->window2, "button", 10, 100)) {
        printf("click! 5\n");
    }

    char *options[] = {
        "option 0",
        "option 1",
        "option 2",
        "option 3",
        "option 4",
        "option 5",
        "option 6",
        "option 7",
        "option 8",
        "option 9",
    };
    s32 option_index = 0;
    _tgui_dropdown_menu(app->window2, 10, 60, options, APP_ARRAY_LEN(options), &option_index, TGUI_ID);
    
    _tgui_dropdown_menu(app->window2, 180, 60, options, APP_ARRAY_LEN(options), &option_index, TGUI_ID);


    tgui_end(painter);

}
