#include "tgui.h"

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
static TGuiDocker root_docker;

/* ---------------------------------- */

void tgui_initialize(void) {
    
    memset(&state, 0, sizeof(TGui));

    arena_initialize(&state.arena, 0, ARENA_TYPE_VIRTUAL);
    virtual_map_initialize(&state.registry);
}

void tgui_terminate(void) {
    virtual_map_terminate(&state.registry);
    arena_terminate(&state.arena);

    memset(&state, 0, sizeof(TGui));
}

/* ---------------------------------- */

#if 1

void tgui_docker_manager_initialize(s32 width, s32 height) {
    memset(&root_docker,0, sizeof(TGuiDocker));
    root_docker.dim = (Rectangle){0, 0, width, height};
}

#if 0
static void tgui_recalculate_child_sizes(TGuiDocker *docker) {

    Rectangle dim = docker->dim;
    s32 abs_x = dim.min_x;
    s32 abs_y = dim.min_y;

    TGuiDocker *child = docker->first_child;
    
    while(child) {
               
        switch(docker->type) {
        
        case TGUI_DOCKER_MANAGER_ROW: {
            if(child->prev) {
                abs_x += child->prev->percentage_of_parent * (dim.max_x - dim.min_x);
            }
            
            child->dim.min_x = abs_x;
            child->dim.max_x = abs_x + child->percentage_of_parent * (dim.max_x - dim.min_x);
            child->dim.min_y = dim.min_y;
            child->dim.max_y = abs_y + dim.max_y;

        } break;
        case TGUI_DOCKER_MANAGER_COLUMN: {
            if(child->prev) {
                abs_y += child->prev->percentage_of_parent * (dim.max_y - dim.min_y);
            }

            child->dim.min_y = abs_y;
            child->dim.max_y = abs_y + child->percentage_of_parent * (dim.max_y - dim.min_y);
            child->dim.min_x = dim.min_x;
            child->dim.max_x = abs_x + dim.max_x;

        } break;
        
        }

        tgui_recalculate_child_sizes(child);
        child = child->next;
    }

}
#endif

void tgui_docker_resize(TGuiDocker *docker, s32 width, s32 height) {
    
    if(!docker) {
        docker = &root_docker;
    }
    
    docker->dim = (Rectangle){0, 0, width, height};
    //tgui_recalculate_child_sizes(docker);
}

void tgui_docker_add_vertical(TGuiDocker *docker) {
    if(!docker) {
        docker = &root_docker;
    }
    ASSERT(docker);
}

void tgui_docker_add_horizontal(TGuiDocker *docker) {
    if(!docker) {
        docker = &root_docker;
    }
    ASSERT(docker);
}


#endif
