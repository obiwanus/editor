#ifndef ED_BASE_H
#define ED_BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_GIF
#define STBI_ASSERT(x)
#include "include/stb_image.h"

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float r32;
typedef double r64;
typedef i32 b32;
typedef unsigned int uint;

#define global static
#define local_persist static

#if BUILD_SLOW
#define assert(Expression) \
  if (!(Expression)) {     \
    *(int *)0 = 0;         \
  }
#else
#define assert(Expression)
#endif

#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define STB_LEAKCHECK_IMPLEMENTATION
#include <include/stb_leakcheck.h>
#include "include/stb_stretchy_buffer.h"

#endif  // ED_BASE_H
