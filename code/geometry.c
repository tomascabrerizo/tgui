#include "geometry.h"

s32 rect_width(Rectangle a) {
    return (a.max_x - a.min_x) + 1;
}

s32 rect_height(Rectangle a) {
    return (a.max_y - a.min_y) + 1;
}

Rectangle rect_intersection(Rectangle a, Rectangle b) {
    Rectangle result;
    result.min_x = MAX(a.min_x, b.min_x);
    result.min_y = MAX(a.min_y, b.min_y);
    result.max_x = MIN(a.max_x, b.max_x);
    result.max_y = MIN(a.max_y, b.max_y);
    return result; 
}

Rectangle rect_union(Rectangle a, Rectangle b) {
    Rectangle result;
    result.min_x = MIN(a.min_x, b.min_x);
    result.min_y = MIN(a.min_y, b.min_y);
    result.max_x = MIN(a.max_x, b.max_x);
    result.max_y = MIN(a.max_y, b.max_y);
    return result; 
}

b32 rect_point_overlaps(Rectangle a, s32 x, s32 y) {
    return (a.min_x <= x && x <= a.max_x && a.min_y <= y && y <= a.max_y); 
}
