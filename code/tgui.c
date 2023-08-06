#include "tgui.h"
#include "common.h"
#include "memory.h"
#include "painter.h"
#include <stdio.h>

u64 hash(u64 key) {
    key = (~key) + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8);
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

static TGui state;
static TGuiDocker docker;
static TGuiInput input;

static Rectangle empty_dim(void) {
    return (Rectangle){0};
}

static TGuiDockerNode *node_alloc(void) {
    TGuiDockerNode *node = NULL;

    if(docker.first_free_node) {
        node = docker.first_free_node;
        docker.first_free_node = docker.first_free_node->next;
    } else {
        node = arena_push_struct(&state.arena, TGuiDockerNode, 8);
    }

    ASSERT(node != NULL);
    memset(node, 0, sizeof(TGuiDockerNode));
    
    return node;
}

static TGuiDockerNode *root_node_alloc(TGuiDockerNode *parent) {
    ASSERT(!parent || parent->type == TGUI_DOCKER_NODE_ROOT);
    
    TGuiDockerNode *node  = node_alloc();
    TGuiDockerNode *dummy = node_alloc();
    
    clink_list_init(dummy);
     
    node->type = TGUI_DOCKER_NODE_ROOT;
    node->childs = dummy;
    node->parent = parent;
    node->dir = TGUI_SPLIT_DIR_NONE;

    return node;
}

TGuiDockerNode *window_node_alloc(TGuiDockerNode *parent) {
    
    TGuiDockerNode *node = node_alloc();

    node->type = TGUI_DOCKER_NODE_WINDOW;
    node->parent = parent;

    return node;
}

TGuiDockerNode *split_node_alloc(TGuiDockerNode *parent, f32 position) {
    ASSERT(parent && parent->type == TGUI_DOCKER_NODE_ROOT);
    
    TGuiDockerNode *node = node_alloc();
    
    node->type = TGUI_DOCKER_NODE_SPLIT;
    node->parent = parent;
    node->split_position = position;
    node->dim = empty_dim();

    return node;
}

void tgui_initialize(void) {
    
    memset(&state, 0, sizeof(TGui));

    arena_initialize(&state.arena, 0, ARENA_TYPE_VIRTUAL);
    virtual_map_initialize(&state.registry);

    memset(&docker, 0, sizeof(TGuiDocker));
    
    docker.root = NULL;
    docker.first_free_node = NULL;
    docker.grabbing_window = false;
    docker.preview_window = (Rectangle){0};
}

void tgui_terminate(void) {
    virtual_map_terminate(&state.registry);
    arena_terminate(&state.arena);

    memset(&state, 0, sizeof(TGui));

}

b32 position_overlap_node(TGuiDockerNode *node, s32 x, s32 y) {
    Rectangle dim = node->dim;
    return (dim.min_x <= x && x <= dim.max_x && dim.min_y <= y && y <= dim.max_y); 
}

TGuiDockerNode *get_node_in_position(TGuiDockerNode *node, s32 x, s32 y) {
    
    if(position_overlap_node(node, x, y)) {
        switch (node->type) {
        
        case TGUI_DOCKER_NODE_ROOT: {
            TGuiDockerNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                TGuiDockerNode *result = get_node_in_position(child, x, y);
                child = child->next; 
                if(result) return result;
            }
        } break;
        case TGUI_DOCKER_NODE_SPLIT: { 
            return node;
        } break;
        case TGUI_DOCKER_NODE_WINDOW: { 
            return node; 
        } break;
        
        }
    
    }

    return 0;
}

void tgui_node_print(TGuiDockerNode *node) {
    switch (node->type) {
    case TGUI_DOCKER_NODE_ROOT:   { printf("TGUI_DOCKER_NODE_ROOT\n");   } break;
    case TGUI_DOCKER_NODE_SPLIT:  { printf("TGUI_DOCKER_NODE_SPLIT\n");  } break;
    case TGUI_DOCKER_NODE_WINDOW: { printf("TGUI_DOCKER_NODE_WINDOW\n"); } break;
    }
}

void root_node_move(TGuiDockerNode *node) {
    ASSERT(node->type == TGUI_DOCKER_NODE_ROOT);
    printf("Moving root node\n");
}

static inline void calculate_mouse_rel_x_y(TGuiDockerNode *split, TGuiDockerNode *parent, f32 *x, f32 *y) {
    
    f32 epsilon = 0.02;

    f32 clamp_max = 1.0f;
    f32 clamp_min = 0.0f;
    
    if(split->next->next->type == TGUI_DOCKER_NODE_SPLIT) clamp_max = split->next->next->split_position;
    if(split->prev->prev->type == TGUI_DOCKER_NODE_SPLIT) clamp_min = split->prev->prev->split_position;
    
    clamp_max -= epsilon;
    clamp_min += epsilon;

    Rectangle parent_dim = parent->dim;
    *x = CLAMP((f32)(input.mouse_x - parent_dim.min_x) / (f32)(parent_dim.max_x - parent_dim.min_x), clamp_min, clamp_max);
    *y = CLAMP((f32)(input.mouse_y - parent_dim.min_y) / (f32)(parent_dim.max_y - parent_dim.min_y), clamp_min, clamp_max);

}

void split_node_move(TGuiDockerNode *node) {
    ASSERT(node->type == TGUI_DOCKER_NODE_SPLIT);
    ASSERT(node->parent != NULL && node->parent->type == TGUI_DOCKER_NODE_ROOT);

    TGuiDockerNode *parent = node->parent;
    
    f32 mouse_rel_x, mouse_rel_y;
    calculate_mouse_rel_x_y(node, parent, &mouse_rel_x, &mouse_rel_y);

    if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {
        node->split_position = mouse_rel_x;
    } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {
        node->split_position = mouse_rel_y;
    }
    
    tgui_node_recalculate_dim(node->prev);
    tgui_node_recalculate_dim(node->next);

}

void node_free(TGuiDockerNode *node) {
    
    if(node->type == TGUI_DOCKER_NODE_ROOT) {
        TGuiDockerNode *dummy = node->childs;

        TGuiDockerNode *child = node->childs;
        while(!clink_list_end(child, node->childs)) {
            TGuiDockerNode *to_free = child;
            child = child->next;
            node_free(to_free);
        }
        
        dummy->next = docker.first_free_node;
        docker.first_free_node = dummy;

    }

    node->next = docker.first_free_node;
    docker.first_free_node = node;

}

TGuiSplitDirection calculate_drop_split_dir(TGuiDockerNode *mouse_over_node) {

        TGuiSplitDirection dir = TGUI_SPLIT_DIR_NONE;
         
        Rectangle dim = mouse_over_node->dim;

        u32 r_distance = dim.max_x - input.mouse_x;
        u32 b_distance = dim.max_y - input.mouse_y;
        
        if(r_distance < b_distance) {
            dir = TGUI_SPLIT_DIR_VERTICAL;
        } else {
            dir = TGUI_SPLIT_DIR_HORIZONTAL;
        }
        
        ASSERT(dir == TGUI_SPLIT_DIR_VERTICAL || dir == TGUI_SPLIT_DIR_HORIZONTAL);
        return dir;
}

static TGuiDockerNode *root_node_get_first_window(TGuiDockerNode *root) {
    ASSERT(root->type == TGUI_DOCKER_NODE_ROOT);
    TGuiDockerNode *child = root->childs->next;
    if(child->type == TGUI_DOCKER_NODE_WINDOW) {
        return child;
    } else if(child->type == TGUI_DOCKER_NODE_ROOT) {
        return root_node_get_first_window(child);
    } else {
        ASSERT(!"Split node cannot be the first child of root node");
        return root;
    }
}

static TGuiDockerNode *split_node_get_first_window(TGuiDockerNode *split) {
    ASSERT(split->type == TGUI_DOCKER_NODE_SPLIT);
    TGuiDockerNode *window = split->next; 
    /* TODO: Find the best way to handle where to place the window when mouse is over a split node */
    if(window->type == TGUI_DOCKER_NODE_ROOT) {
        window = root_node_get_first_window(window);
    } 
    ASSERT(window->type == TGUI_DOCKER_NODE_WINDOW);
    return window;
}

void calculate_preview_split(TGuiDockerNode *mouse_over_node) {
        
        TGuiDockerNode *node = mouse_over_node;
        TGuiSplitDirection dir = calculate_drop_split_dir(node);
        
        if(node->type == TGUI_DOCKER_NODE_SPLIT) {
            node = split_node_get_first_window(mouse_over_node);
            dir = TGUI_SPLIT_DIR_HORIZONTAL;
        }

        ASSERT(node->type == TGUI_DOCKER_NODE_WINDOW || node->type == TGUI_DOCKER_NODE_ROOT);
        
        docker.preview_window = node->dim;
        ASSERT(dir != TGUI_SPLIT_DIR_NONE);
        
        if(dir == TGUI_SPLIT_DIR_VERTICAL) {
            docker.preview_window.min_x = (node->dim.max_x + node->dim.min_x) / 2; 
        } else if(dir == TGUI_SPLIT_DIR_HORIZONTAL) {
            docker.preview_window.min_y = (node->dim.max_y + node->dim.min_y) / 2; 
        }

}

static Rectangle calculate_menu_bar_rect(TGuiDockerNode *window) {
    ASSERT(window->type == TGUI_DOCKER_NODE_WINDOW);

    Rectangle menu_bar_rect = window->dim;
    menu_bar_rect.min_x += 1;
    menu_bar_rect.max_x -= 1;
    menu_bar_rect.min_y += 1;
    menu_bar_rect.max_y = menu_bar_rect.min_y + MENU_BAR_HEIGHT;

    return menu_bar_rect;
}

static b32 mouse_in_menu_bar(TGuiDockerNode *window) {
    ASSERT(window->type == TGUI_DOCKER_NODE_WINDOW);

    s32 x = input.mouse_x;
    s32 y = input.mouse_y;
    Rectangle dim = calculate_menu_bar_rect(window);

    return (dim.min_x <= x && x <= dim.max_x && dim.min_y <= y && y <= dim.max_y);
}

void window_grabbing_start(TGuiDockerNode *window) {
    
    if(docker.grabbing_window) {
        TGuiDockerNode *mouse_over_node = get_node_in_position(docker.root, input.mouse_x, input.mouse_y);
        if(mouse_over_node) {
            calculate_preview_split(mouse_over_node);
        }
        return;
    } else {
        /* TODO: This is a complete hack, program should only start the window grabbing if the click start from the menu bar */
        if(!mouse_in_menu_bar(window)) return;
    }
    
    docker.grabbing_window = true;

    TGuiDockerNode *parent = window->parent;
    if(!parent) return;
    
    TGuiDockerNode *split = NULL;

    if(clink_list_first(window, parent->childs) && clink_list_last(window, parent->childs)) {
        ASSERT(!"Root nodes cannot have only one child");
    } else if(clink_list_first(window, parent->childs)) {
        split = window->next;
    } else if(clink_list_last(window, parent->childs)) {
        split = window->prev;
    } else {
        split = window->next;
    }

    ASSERT(split != NULL && split->type == TGUI_DOCKER_NODE_SPLIT);
    clink_list_remove(window);
    clink_list_remove(split);
    node_free(split);
    
    docker.active_node = window;

    /* NOTE: If the parent is left with just one child, we need to replace the parent node with the only child */
    TGuiDockerNode *child = parent->childs->next;
    if(clink_list_first(child, parent->childs) && clink_list_last(child, parent->childs)) {
        
        if(parent == docker.root) {
            tgui_set_root_node(child);
            node_free(parent);
            parent = child;
        } else {
            child->parent = parent->parent;

            clink_list_remove(child);
            clink_list_insert_front(parent, child);
            clink_list_remove(parent);
            node_free(parent);
            parent = child->parent;
        }
    }
     
    tgui_node_recalculate_dim(parent);

}

void window_grabbing_end(TGuiDockerNode *node) {

    TGuiDockerNode *mouse_over_node = get_node_in_position(docker.root, input.mouse_x, input.mouse_y);
    if(mouse_over_node) {
        
        TGuiSplitDirection dir = calculate_drop_split_dir(mouse_over_node);

        if(mouse_over_node->type == TGUI_DOCKER_NODE_SPLIT) {
            mouse_over_node = split_node_get_first_window(mouse_over_node);
            dir = TGUI_SPLIT_DIR_HORIZONTAL;
        }
        tgui_node_split(mouse_over_node, dir, node);
    }

    tgui_node_recalculate_dim(node->parent);

    docker.active_node = NULL;
    docker.grabbing_window = false;
    docker.preview_window = (Rectangle){0};
}

void window_node_move(TGuiDockerNode *node) {
    ASSERT(node->type == TGUI_DOCKER_NODE_WINDOW);
    window_grabbing_start(node);
}

void set_cursor_state(TGuiDockerNode *mouse_over) {
    
    /* TODO: Should probably handle the situation when we are performing an action i a better way */
    if(docker.active_node) return;

    state.cursor = TGUI_CURSOR_ARROW;

    
    if(!mouse_over) {
        state.cursor = TGUI_CURSOR_ARROW;
        return;
    }

    switch (mouse_over->type) {
    
    case TGUI_DOCKER_NODE_ROOT: {
        state.cursor = TGUI_CURSOR_ARROW;
    } break; 
    case TGUI_DOCKER_NODE_WINDOW: {
        if(mouse_in_menu_bar(mouse_over)) {
            state.cursor = TGUI_CURSOR_HAND;
        } else {
            state.cursor = TGUI_CURSOR_ARROW;
        }
    } break; 
    case TGUI_DOCKER_NODE_SPLIT: {
        ASSERT(mouse_over->parent);
        TGuiDockerNode *parent = mouse_over->parent;
        if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {
            state.cursor = TGUI_CURSOR_H_ARROW;
        } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {
            state.cursor = TGUI_CURSOR_V_ARROW;
        }
    } break; 
    
    }

}

void tgui_update(void) {
    if(input.window_resize) {
        tgui_node_recalculate_dim(docker.root);
        input.window_resize = false;
    }
    
    TGuiDockerNode *mouse_over_node = get_node_in_position(docker.root, input.mouse_x, input.mouse_y);

    set_cursor_state(mouse_over_node);

    if(mouse_over_node && (mouse_over_node->type == TGUI_DOCKER_NODE_SPLIT || mouse_over_node->type == TGUI_DOCKER_NODE_WINDOW)) {
        if(!docker.active_node && input.mouse_button_is_down && !input.mouse_button_was_down) {
            docker.active_node = mouse_over_node;
        }
    }
    
    if(!input.mouse_button_is_down) {
        if(docker.grabbing_window) {
            window_grabbing_end(docker.active_node);
        }
        docker.active_node = NULL;
    }

    if(docker.active_node != NULL) {
    
    switch (docker.active_node->type) {
    case TGUI_DOCKER_NODE_ROOT:   { root_node_move(docker.active_node);   } break;
    case TGUI_DOCKER_NODE_SPLIT:  { split_node_move(docker.active_node);  } break;
    case TGUI_DOCKER_NODE_WINDOW: { window_node_move(docker.active_node); } break;
    }
        
    }
    
    input.mouse_button_was_down = input.mouse_button_is_down;

}

TGuiInput *tgui_get_input(void) {
    return &input;
}

void tgui_set_root_node(TGuiDockerNode *node) {
    ASSERT(node->type == TGUI_DOCKER_NODE_ROOT || node->type == TGUI_DOCKER_NODE_WINDOW);
    docker.root = node;
    node->parent = NULL;
}

void tgui_root_set_child(TGuiDockerNode *root, TGuiDockerNode *node) {
    ASSERT(root->type == TGUI_DOCKER_NODE_ROOT);
    ASSERT(node->type == TGUI_DOCKER_NODE_WINDOW || node->type == TGUI_DOCKER_NODE_ROOT);
    
    ASSERT(clink_list_is_empty(root->childs));
    if(clink_list_is_empty(root->childs)) {
        node->parent = root;
        clink_list_insert_front(root->childs, node);
    }

}

void tgui_node_split(TGuiDockerNode *node, TGuiSplitDirection dir, TGuiDockerNode *node1) {
    ASSERT(node->type == TGUI_DOCKER_NODE_WINDOW || node->type == TGUI_DOCKER_NODE_ROOT);
    
    TGuiDockerNode *parent = node->parent;
    
    /* NOTE: The only case this can happend is if we have a window as the root */
    if(!parent) {
        ASSERT(docker.root == node);
        TGuiDockerNode *root = root_node_alloc(0);
        tgui_set_root_node(root);
        tgui_root_set_child(root, node);
        parent = root;
    }

    ASSERT(parent && parent->type == TGUI_DOCKER_NODE_ROOT);
    
    if(parent->dir == TGUI_SPLIT_DIR_NONE) {
        parent->dir = dir;
    } else if(parent->dir != dir) {
        
        TGuiDockerNode *root = root_node_alloc(parent);
        root->dir = dir;
        clink_list_insert_back(node, root);
        clink_list_remove(node);
        tgui_root_set_child(root, node);
        parent = root;
    
    }

    node1->parent = parent;

    f32 split_position = 0.0f;
    
    if(clink_list_first(node, parent->childs) && clink_list_last(node, parent->childs)) {
        split_position = 0.5f;
    } else if(clink_list_first(node, parent->childs)) {
    
        TGuiDockerNode *split_next = node->next;
        ASSERT(split_next->type == TGUI_DOCKER_NODE_SPLIT);
        split_position = split_next->split_position * 0.5f;
    
    } else if(clink_list_last(node, parent->childs)) {
    
        TGuiDockerNode *split_prev = node->prev;
        ASSERT(split_prev->type == TGUI_DOCKER_NODE_SPLIT);
        split_position = split_prev->split_position + ((1 - split_prev->split_position) * 0.5f);
    
    } else {
        
        TGuiDockerNode *split_next = node->next;
        TGuiDockerNode *split_prev = node->prev;
        ASSERT(split_next->type == TGUI_DOCKER_NODE_SPLIT);
        ASSERT(split_prev->type == TGUI_DOCKER_NODE_SPLIT);
        split_position = split_prev->split_position + ((split_next->split_position - split_prev->split_position) * 0.5f);
    
    }

    ASSERT(split_position >= 0.0f && split_position <= 1.0f);

    TGuiDockerNode *split = split_node_alloc(node->parent, split_position);
    clink_list_insert_front(node, split);
    clink_list_insert_front(split, node1);

}

void split_recalculate_dim(TGuiDockerNode *split) {
    ASSERT(split->type == TGUI_DOCKER_NODE_SPLIT);
    ASSERT(split->parent && split->parent->type == TGUI_DOCKER_NODE_ROOT);


    TGuiDockerNode *parent = split->parent;
    ASSERT(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL || parent->dir == TGUI_SPLIT_DIR_VERTICAL);
    
    Rectangle dim = parent->dim;
    split->dim = dim;
    
    if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {
        
        s32 abs_split_position = dim.min_x + (split->split_position * (dim.max_x - dim.min_x));
        split->dim.min_x =  abs_split_position - SPLIT_HALF_SIZE;
        split->dim.max_x =  abs_split_position + SPLIT_HALF_SIZE;
    
    } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {

        s32 abs_split_position = dim.min_y + (split->split_position * (dim.max_y - dim.min_y));
        split->dim.min_y =  abs_split_position - SPLIT_HALF_SIZE;
        split->dim.max_y =  abs_split_position + SPLIT_HALF_SIZE;
    }
     
}

void tgui_node_recalculate_dim(TGuiDockerNode *node) {

    ASSERT(node->type == TGUI_DOCKER_NODE_WINDOW || node->type == TGUI_DOCKER_NODE_ROOT);

    TGuiDockerNode *parent = node->parent;
    
    if(parent) {
       
        node->dim = parent->dim;

        ASSERT(parent->type == TGUI_DOCKER_NODE_ROOT);

        if(clink_list_first(node, parent->childs) && clink_list_last(node, parent->childs)) {
            
            ASSERT(!"Root nodes cannot hace only one child");
         
        } else if(clink_list_first(node, parent->childs)) {
            ASSERT(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL || parent->dir == TGUI_SPLIT_DIR_VERTICAL);
            
            TGuiDockerNode *split_next = node->next;
            ASSERT(split_next->type == TGUI_DOCKER_NODE_SPLIT);
            split_recalculate_dim(split_next);

            if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {
                
                node->dim.min_x = parent->dim.min_x;
                node->dim.max_x = split_next->dim.min_x;
            
            } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {

                node->dim.min_y = parent->dim.min_y;
                node->dim.max_y = split_next->dim.min_y;
            }

        } else if(clink_list_last(node, parent->childs)) {
            ASSERT(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL || parent->dir == TGUI_SPLIT_DIR_VERTICAL);

            TGuiDockerNode *split_prev = node->prev;
            ASSERT(split_prev->type == TGUI_DOCKER_NODE_SPLIT);
            split_recalculate_dim(split_prev);

            if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {
            
                node->dim.min_x = split_prev->dim.max_x; 
                node->dim.max_x = parent->dim.max_x;

            } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {

                node->dim.min_y = split_prev->dim.max_y; 
                node->dim.max_y = parent->dim.max_y;
            }
        
        } else {
            ASSERT(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL || parent->dir == TGUI_SPLIT_DIR_VERTICAL);

            TGuiDockerNode *split_next = node->next;
            TGuiDockerNode *split_prev = node->prev;
            ASSERT(split_next->type == TGUI_DOCKER_NODE_SPLIT);
            ASSERT(split_prev->type == TGUI_DOCKER_NODE_SPLIT);
            split_recalculate_dim(split_next);
            split_recalculate_dim(split_prev);

            if(parent->dir == TGUI_SPLIT_DIR_VERTICAL) {

                node->dim.min_x = split_prev->dim.max_x; 
                node->dim.max_x = split_next->dim.min_x;

            } else if(parent->dir == TGUI_SPLIT_DIR_HORIZONTAL) {

                node->dim.min_y = split_prev->dim.max_y; 
                node->dim.max_y = split_next->dim.min_y;
            }
        }
    } else {
        node->dim = (Rectangle){0, 0, input.resize_w, input.resize_h};
    }

    if(node->type == TGUI_DOCKER_NODE_ROOT) {
       
        TGuiDockerNode *child = node->childs->next;
        while (!clink_list_end(child, node->childs)) { 

            /* NOTE: For now split node dim are calculated online when needed */
            /* TODO: Maybe add dirty flag to actually use the cashed dimension */
            if(child->type == TGUI_DOCKER_NODE_SPLIT) {
                child = child->next;
            }
            
            tgui_node_recalculate_dim(child);

            child = child->next;
        }

    }

}

TGuiCursor tgui_get_cursor_state(void) {
    return state.cursor;
}

static inline void draw_rectangle(Painter *painter, Rectangle rect, u32 color) {
    s32 x = rect.min_x;
    s32 y = rect.min_y;
    s32 w = (rect.max_x - rect.min_x);
    s32 h = (rect.max_y - rect.min_y);
    painter_draw_rect(painter, x, y, w, h, color);
}

void node_draw(Painter *painter, TGuiDockerNode *node) {

    switch (node->type) {
     
    case TGUI_DOCKER_NODE_WINDOW: {
        draw_rectangle(painter, node->dim, 0x777777);
        
        u32 border_color = 0xbbbbbb;
        painter_draw_hline(painter, node->dim.min_y,   node->dim.min_x, node->dim.max_x, border_color);
        painter_draw_hline(painter, node->dim.max_y-1, node->dim.min_x, node->dim.max_x, border_color);
        painter_draw_vline(painter, node->dim.min_x,   node->dim.min_y, node->dim.max_y, border_color);
        painter_draw_vline(painter, node->dim.max_x-1, node->dim.min_y, node->dim.max_y, border_color);

        u32 menu_bar_color = 0x555555;
        Rectangle menu_bar_rect = calculate_menu_bar_rect(node);
        draw_rectangle(painter, menu_bar_rect, menu_bar_color);
    } break;

    case TGUI_DOCKER_NODE_SPLIT: {
        draw_rectangle(painter, node->dim, 0x444444);
    } break;


    case TGUI_DOCKER_NODE_ROOT: {
        TGuiDockerNode * child = node->childs->next;
        while (!clink_list_end(child, node->childs)) { 
            node_draw(painter, child);
            child = child->next;
        }
    } break;

    }
}

void root_node_draw(Painter *painter) {
    node_draw(painter, docker.root);

    if(docker.grabbing_window) {
        u32 border_color = 0xaaaaff;
        Rectangle dim = docker.preview_window;
        painter_draw_hline(painter,dim.min_y,   dim.min_x, dim.max_x, border_color);
        painter_draw_hline(painter,dim.max_y-1, dim.min_x, dim.max_x, border_color);
        painter_draw_vline(painter,dim.min_x,   dim.min_y, dim.max_y, border_color);
        painter_draw_vline(painter,dim.max_x-1, dim.min_y, dim.max_y, border_color);
    }

}
