#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include "common.h"

typedef struct Rectangle {
    s32 min_x, min_y;
    s32 max_x, max_y;
} Rectangle;

s32 rect_width(Rectangle a);

s32 rect_height(Rectangle a);

Rectangle rect_intersection(Rectangle a, Rectangle b);

Rectangle rect_union(Rectangle a, Rectangle b);

b32 rect_point_overlaps(Rectangle a, s32 x, s32 y);

#endif /* _GEOMETRY_H_ */
