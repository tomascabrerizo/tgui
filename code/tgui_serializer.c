#include "tgui_serializer.h"
#include "common.h"
#include "memory.h"
#include "tgui_docker.h"
#include "tgui.h"

#include <stdio.h>
#include <stdarg.h>

extern TGui state;
extern TGuiDocker docker;

static FILE *file = NULL;

static inline void indent(FILE *stream, u32 n) {
    for(u32 i = 0; i < n; ++i) {
        fprintf(stream, "  ");
    }
}

static inline void write_to_file(u32 depth, char *str, ...) {
    va_list argp;
    va_start(argp, str);
    indent(file, depth); vfprintf(file, str, argp);
    va_end(argp);
}

void tgui_serializer_write_window(TGuiWindow *window, u32 depth) {
    write_to_file(depth, "window: {\n");
    write_to_file(depth+1, "name: %s\n", window->name);
    write_to_file(depth+1, "is_scrolling %d\n", window->is_scrolling);
    write_to_file(depth, "}\n");
}

void tgui_serializer_write_node(TGuiDockerNode *node, u32 depth) {

    switch(node->type) {
    
        case TGUI_DOCKER_NODE_WINDOW: {
            write_to_file(depth, "window_node: {\n");
            write_to_file(depth+1, "active_window: %s\n", node->active_window->name);
            write_to_file(depth+1, "window_count: %d\n", node->windows_count);
            TGuiWindow *window = node->windows->next;
            while (!clink_list_end(window, node->windows)) {
                tgui_serializer_write_window(window, depth+1);
                window = window->next;
            }
            write_to_file(depth, "}\n");

        } break;

        case TGUI_DOCKER_NODE_SPLIT: {
            write_to_file(depth, "split_node: {\n");
            write_to_file(depth+1, "split_position: %f\n", node->split_position);
            write_to_file(depth, "}\n");
        } break;

        case TGUI_DOCKER_NODE_ROOT: {
            write_to_file(depth, "root_node: {\n");
            write_to_file(depth+1, "dir %d\n", node->dir);
            TGuiDockerNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                tgui_serializer_write_node(child, depth+1);
                child = child->next;
            }
            write_to_file(depth, "}\n");

        } break;
    }
}

void tgui_serializer_write_docker_tree(void) {
    ASSERT(docker.root);

    file = fopen("./tgui.dat", "w");
    fseek(file, 0, SEEK_SET);
    tgui_serializer_write_node(docker.root, 0);
    fclose(file);

}
