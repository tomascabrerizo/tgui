#ifndef _TGUI_COMMON_H_
#define _TGUI_COMMON_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#define TGUI_ASSERT(expr) assert((expr))
#define TGUI_UNUSED(var) ((void)(var))
#define TGUI_IS_POWER_OF_TWO(expr) (((expr) & (expr - 1)) == 0)

#define TGUI_MAX(a, b) ((a) >= (b) ? (a) : (b))
#define TGUI_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define TGUI_MAX3(a, b, c) TGUI_MAX(TGUI_MAX(a, b), c)
#define TGUI_MIN3(a, b, c) TGUI_MIN(TGUI_MIN(a, b), c)

#define TGUI_ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define TGUI_CLAMP(value, min, max) TGUI_MAX(TGUI_MIN(value, max), min)

#define TGUI_OFFSET_OF(type, value) (&(((type *)0)->value))

typedef unsigned long long tgui_u64;
typedef unsigned int       tgui_u32;
typedef unsigned short     tgui_u16;
typedef unsigned char      tgui_u8;

typedef long long          tgui_s64;
typedef int                tgui_s32;
typedef short              tgui_s16;
typedef char               tgui_s8;

typedef float              tgui_f32;
typedef double             tgui_f64;

typedef unsigned int       tgui_b32;
typedef unsigned char      tgui_b8;

#define true  1
#define false 0

#endif /* _TGUI_COMMON_H_ */
