#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "common.h"

typedef struct Rectangle {
    s32 min_x, min_y;
    s32 max_x, max_y;
} Rectangle;

Rectangle rect_from_wh(u32 x, u32 y, u32 w, u32 h);

s32 rect_width(Rectangle a);

s32 rect_height(Rectangle a);

Rectangle rect_intersection(Rectangle a, Rectangle b);

Rectangle rect_union(Rectangle a, Rectangle b);

b32 rect_inside(Rectangle a, Rectangle b);

b32 rect_equals(Rectangle a, Rectangle b);

b32 rect_invalid(Rectangle a);

b32 rect_point_overlaps(Rectangle a, s32 x, s32 y);

Rectangle rect_set_invalid(void);

#endif /* _GEOMETRY_H_ */
