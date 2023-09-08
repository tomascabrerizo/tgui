#ifndef _TGUI_GEOMETRY_H_
#define _TGUI_GEOMETRY_H_

#include "tgui_common.h"

typedef struct TGuiRectangle {
    tgui_s32 min_x, min_y;
    tgui_s32 max_x, max_y;
} TGuiRectangle;

TGuiRectangle tgui_rect_from_wh(tgui_u32 x, tgui_u32 y, tgui_u32 w, tgui_u32 h);

tgui_s32 tgui_rect_width(TGuiRectangle a);

tgui_s32 tgui_rect_height(TGuiRectangle a);

TGuiRectangle tgui_rect_intersection(TGuiRectangle a, TGuiRectangle b);

TGuiRectangle tgui_rect_union(TGuiRectangle a, TGuiRectangle b);

tgui_b32 tgui_rect_inside(TGuiRectangle a, TGuiRectangle b);

tgui_b32 tgui_rect_equals(TGuiRectangle a, TGuiRectangle b);

tgui_b32 tgui_rect_invalid(TGuiRectangle a);

tgui_b32 tgui_rect_point_overlaps(TGuiRectangle a, tgui_s32 x, tgui_s32 y);

TGuiRectangle tgui_rect_set_invalid(void);

TGuiRectangle tgui_rect_translate(TGuiRectangle rect, tgui_s32 x, tgui_s32 y);

void tgui_rect_print(TGuiRectangle rect);

#endif /* _TGUI_GEOMETRY_H_ */
