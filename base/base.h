#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

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
typedef i32 bool32;
typedef unsigned int uint;

#define global static
#define local_persist static

#if BUILD_SLOW
#define Assert(Expression) \
  if (!(Expression)) {     \
    *(int *)0 = 0;         \
  }
#else
#define Assert(Expression)
#endif

#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#endif  // EDITOR_BASE_H
