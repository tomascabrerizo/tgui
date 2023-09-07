#ifndef _TGUI_GEOMETRY_H_
#define _TGUI_GEOMETRY_H_

#include "common.h"

typedef struct TGuiRectangle {
    s32 min_x, min_y;
    s32 max_x, max_y;
} TGuiRectangle;

TGuiRectangle tgui_rect_from_wh(u32 x, u32 y, u32 w, u32 h);

s32 tgui_rect_width(TGuiRectangle a);

s32 tgui_rect_height(TGuiRectangle a);

TGuiRectangle tgui_rect_intersection(TGuiRectangle a, TGuiRectangle b);

TGuiRectangle tgui_rect_union(TGuiRectangle a, TGuiRectangle b);

b32 tgui_rect_inside(TGuiRectangle a, TGuiRectangle b);

b32 tgui_rect_equals(TGuiRectangle a, TGuiRectangle b);

b32 tgui_rect_invalid(TGuiRectangle a);

b32 tgui_rect_point_overlaps(TGuiRectangle a, s32 x, s32 y);

TGuiRectangle tgui_rect_set_invalid(void);

TGuiRectangle tgui_rect_translate(TGuiRectangle rect, s32 x, s32 y);

void tgui_rect_print(TGuiRectangle rect);

#endif /* _TGUI_GEOMETRY_H_ */
