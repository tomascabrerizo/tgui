#ifndef _TGUI_DOCKER_H_
#define _TGUI_DOCKER_H_

#include "tgui_painter.h"

#define SPLIT_HALF_SIZE 1
#define MENU_BAR_HEIGHT 20

struct TGuiWindow;
struct TGuiAllocatedWindow;

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

typedef enum TGuiPosition {
    TGUI_POS_FRONT,
    TGUI_POS_BACK,
} TGuiPosition;

typedef struct TGuiDockerNode {
    TGuiRectangle dim;
    
    TGuiDockerNodeType type;

    struct TGuiDockerNode *parent;
    struct TGuiDockerNode *prev;
    struct TGuiDockerNode *next;

    float split_position; /* Only for split nodes */
    TGuiSplitDirection dir; /* Only for root nodes */
    struct TGuiDockerNode *childs; /* Only for root nodes   */
    
    /* Only for window nodes */
    tgui_u32 active_window;
    struct TGuiWindow *windows;
    tgui_u32 windows_count;
    
    /* Only a dummy window to use as head of double link list of windows */
    struct TGuiAllocatedWindow *dummy_allocated_window;

} TGuiDockerNode;

TGuiDockerNode *node_alloc(void);

TGuiDockerNode *root_node_alloc(TGuiDockerNode *parent);

TGuiDockerNode *window_node_alloc(TGuiDockerNode *parent);
    
TGuiDockerNode *split_node_alloc(TGuiDockerNode *parent, tgui_f32 position);

void node_free(TGuiDockerNode *node);

char *tgui_docker_node_to_str(TGuiDockerNodeType type);

typedef struct TGuiDocker {
    TGuiDockerNode *root;
    TGuiDockerNode *first_free_node;

    TGuiDockerNode *active_node;
    tgui_b32 grabbing_window;
    tgui_b32 grabbing_window_start;
    TGuiRectangle preview_window;

    tgui_b32 saved_mouse_x;
    tgui_b32 saved_mouse_y;

} TGuiDocker;

void tgui_docker_initialize(void);

void tgui_docker_terminate(void);

void tgui_docker_update(void);

void tgui_docker_set_root_node(TGuiDockerNode *node);

void tgui_docker_root_set_child(TGuiDockerNode *root, TGuiDockerNode *node);

void tgui_docker_node_split(TGuiDockerNode *node, TGuiSplitDirection dir, TGuiPosition pos, TGuiDockerNode *node1);

void tgui_docker_node_recalculate_dim(TGuiDockerNode *node);

void tgui_docker_window_node_add_window(TGuiDockerNode *window_node, struct TGuiWindow *window);

void tgui_docker_window_node_remove_window(struct TGuiWindow *window);

tgui_b32 tgui_docker_window_has_tabs(TGuiDockerNode *window_node);


void tgui_docker_root_node_draw(TGuiPainter *painter);

void tgui_docker_draw_preview(TGuiPainter *painter);

TGuiRectangle tgui_docker_get_client_rect(TGuiDockerNode *window);

tgui_b32 tgui_docker_window_is_visible(TGuiDockerNode *window_node, struct TGuiWindow *window);


TGuiDockerNode *node_alloc(void);

TGuiDockerNode *root_node_alloc(TGuiDockerNode *parent);

TGuiDockerNode *window_node_alloc(TGuiDockerNode *parent);

TGuiDockerNode *split_node_alloc(TGuiDockerNode *parent, tgui_f32 position);

#endif /* _TGUI_DOCKER_H_ */
