#ifndef _TGUI_DOCKER_H_
#define _TGUI_DOCKER_H_

#include "common.h"
#include "memory.h"
#include "painter.h"

#define SPLIT_HALF_SIZE 3
#define MENU_BAR_HEIGHT 20

typedef enum TGuiDockerNodeType {
    TGUI_DOCKER_NODE_ROOT = 0,
    TGUI_DOCKER_NODE_WINDOW,
    TGUI_DOCKER_NODE_SPLIT,
} TGuiDockerNodeType;

typedef enum TGuiSplitDirection {
    TGUI_SPLIT_DIR_NONE,
    TGUI_SPLIT_DIR_VERTICAL,
    TGUI_SPLIT_DIR_HORIZONTAL,
} TGuiSplitDirection;

typedef struct TGuiDockerNode {
    Rectangle dim;
    
    TGuiDockerNodeType type;

    struct TGuiDockerNode *parent;
    struct TGuiDockerNode *prev;
    struct TGuiDockerNode *next;

    float split_position; /* Only for split nodes */
    TGuiSplitDirection dir; /* Only for root nodes */
    struct TGuiDockerNode *childs; /* Only for root nodes   */
    struct TGuiWindow *window;     /* Only for window nodes */

} TGuiDockerNode;

typedef struct TGuiDocker {
    TGuiDockerNode *root;
    TGuiDockerNode *first_free_node;

    TGuiDockerNode *active_node;
    b32 grabbing_window;
    Rectangle preview_window;

} TGuiDocker;

void tgui_docker_initialize(void);

void tgui_docker_terminate(void);

void tgui_docker_update(void);

void tgui_docker_set_root_node(TGuiDockerNode *node);

void tgui_docker_root_set_child(TGuiDockerNode *root, TGuiDockerNode *node);

void tgui_docker_node_split(TGuiDockerNode *node, TGuiSplitDirection dir, TGuiDockerNode *node1);

void tgui_docker_node_recalculate_dim(TGuiDockerNode *node);

TGuiDockerNode *tgui_docker_create_root_window(void);

TGuiDockerNode *tgui_docker_split_window(TGuiDockerNode *window, TGuiSplitDirection dir);

void tgui_docker_root_node_draw(Painter *painter);

#endif /* _TGUI_DOCKER_H_ */
