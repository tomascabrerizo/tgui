#include "tgui_geometry.h"

TGuiRectangle tgui_rect_from_wh(u32 x, u32 y, u32 w, u32 h) {
    TGuiRectangle result;
    result.min_x = x;
    result.max_x = x + w - 1;
    result.min_y = y;
    result.max_y = y + h - 1;
    return result;
}

s32 tgui_rect_width(TGuiRectangle a) {
    return (a.max_x - a.min_x) + 1;
}

s32 tgui_rect_height(TGuiRectangle a) {
    return (a.max_y - a.min_y) + 1;
}

b32 tgui_rect_inside(TGuiRectangle a, TGuiRectangle b) {
    return ((a.min_x >= b.min_x) && (a.min_y >= b.min_y) && (a.max_x <= b.max_x) && (a.max_y <= b.max_y));
}

b32 tgui_rect_equals(TGuiRectangle a, TGuiRectangle b) {
    return ((a.min_x == b.min_x) &&  (a.max_x == b.max_x) && (a.min_y == b.min_y) && (a.max_y == b.max_y));
}

b32 tgui_rect_invalid(TGuiRectangle a) {
    return ((a.max_x < a.min_x) || (a.max_y < a.min_y));
}

TGuiRectangle tgui_rect_intersection(TGuiRectangle a, TGuiRectangle b) {
    TGuiRectangle result;
    result.min_x = MAX(a.min_x, b.min_x);
    result.min_y = MAX(a.min_y, b.min_y);
    result.max_x = MIN(a.max_x, b.max_x);
    result.max_y = MIN(a.max_y, b.max_y);
    return result; 
}

TGuiRectangle tgui_rect_union(TGuiRectangle a, TGuiRectangle b) {
    TGuiRectangle result;
    result.min_x = MIN(a.min_x, b.min_x);
    result.min_y = MIN(a.min_y, b.min_y);
    result.max_x = MAX(a.max_x, b.max_x);
    result.max_y = MAX(a.max_y, b.max_y);
    return result; 
}

b32 tgui_rect_point_overlaps(TGuiRectangle a, s32 x, s32 y) {
    return (a.min_x <= x && x <= a.max_x && a.min_y <= y && y <= a.max_y); 
}

TGuiRectangle tgui_rect_set_invalid(void) {
    TGuiRectangle result = (TGuiRectangle){1, 1, -1, -1};
    ASSERT(tgui_rect_invalid(result));
    return result;
}

TGuiRectangle tgui_rect_translate(TGuiRectangle rect, s32 x, s32 y) {
    TGuiRectangle result;
    result.min_x = x;
    result.min_y = y;
    result.max_x = tgui_rect_width(rect)-1;
    result.max_y = tgui_rect_height(rect)-1;
    return result;
}

void rect_print(TGuiRectangle rect) {
    printf("min_x:%d, min_y:%d, max_x:%d, max_y:%d\n", rect.min_x, rect.min_y, rect.max_x, rect.max_y);
}
