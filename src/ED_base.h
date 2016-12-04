#ifndef ED_BASE_H
#define ED_BASE_H

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

// 256 Mb
#define MAX_INTERNAL_MEMORY_SIZE (256 * 1024 * 1024)

struct Program_Memory {
  void *free_memory;
  size_t allocated;

  // TODO: do something with allocations finally
  void *allocate(size_t);
};

#endif  // ED_BASE_H
