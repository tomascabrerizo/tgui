#ifndef _TGUI_H_
#define _TGUI_H_

#include "common.h"
#include "memory.h"
#include "painter.h"

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

#define SPLIT_HALF_SIZE 3
#define MENU_BAR_HEIGHT 20

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

typedef struct TGuiInput {
    b32 mouse_button_is_down;
    b32 mouse_button_was_down;
    
    s32 mouse_x;
    s32 mouse_y;
    
    b32 window_resize;
    s32 resize_w;
    s32 resize_h;

} TGuiInput;

typedef struct TGui {
    Arena arena;
    VirtualMap registry;

    u64 hot;
    u64 active;
} TGui;

void tgui_initialize(void);

void tgui_terminate(void);

void tgui_update(void);

TGuiInput *tgui_get_input(void);

void tgui_set_root_node(TGuiDockerNode *node);

void tgui_root_set_child(TGuiDockerNode *root, TGuiDockerNode *node);

void tgui_node_split(TGuiDockerNode *node, TGuiSplitDirection dir, TGuiDockerNode *node1);

void tgui_node_recalculate_dim(TGuiDockerNode *node);

#endif /* _TGUI_H_ */
