#ifndef _COMMON_H_
#define _COMMON_H_

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#define ASSERT(expr) assert((expr))
#define UNUSED(var) ((void)(var))
#define IS_POWER_OF_TWO(expr) (((expr) & (expr - 1)) == 0)
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define CLAMP(value, min, max) MAX(MIN(value, max), min)

typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;

typedef long long s64;
typedef int       s32;
typedef short     s16;
typedef char      s8;

typedef float  f32;
typedef double f64;

typedef unsigned int  b32;
typedef unsigned char b8;

#define true  1
#define false 0


#endif /* _COMMON_H_ */
