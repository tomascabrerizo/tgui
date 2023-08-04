#ifndef _TGUI_H_
#define _TGUI_H_

#include "common.h"
#include "memory.h"
#include "painter.h"

typedef struct TGui {
    Arena arena;
    VirtualMap registry;

    u64 hot;
    u64 active;

} TGui;

typedef struct TGuiWindow {
    Rectangle dim;
    Rectangle clip;
} TGuiWindow;

typedef struct TGuiDocker{

    Rectangle dim;
    union TGuiContainer *first_child;
    union TGuiContainer *last_child;

} TGuiDocker;

typedef enum TGuiContainerType {
    TGUI_CONTAINER_SPLITTER_VERTICAL,
    TGUI_CONTAINER_SPLITTER_HORIZONTAL,
    TGUI_CONTAINER_WINDOW,
} TGuiContainerType;

#define TGUI_CONTAINER_ELEMENT \
    TGuiContainerType type;    \
    struct TGuiDocker *parent; \
    union TGuiContainer *next; \
    union TGuiContainer *prev;

typedef struct TGuiContainerSplitter {
    TGUI_CONTAINER_ELEMENT

    float percentage;
    TGuiDocker *first;
    TGuiDocker *second;

} TGuiContainerSplitter;

typedef struct TGuiContainerWindow {
    TGUI_CONTAINER_ELEMENT

    TGuiWindow *window;

} TGuiContainerWindow;

typedef union TGuiContainer {
    TGuiContainerType type;
    TGuiContainerSplitter splitter;
    TGuiContainerWindow window;
} TGuiContainer;


void tgui_initialize(void);
void tgui_terminate(void);

#endif /* _TGUI_H_ */
