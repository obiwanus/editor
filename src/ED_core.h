#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_ui.h"
#include "raytrace/ED_raytrace.h"

// 512 Mb
#define MAX_INTERNAL_MEMORY_SIZE (512 * 1024 * 1024)

struct Program_Memory {
  void *free_memory;
  size_t allocated;

  void *allocate(size_t);
};

Update_Result update_and_render(Program_Memory *, Pixel_Buffer *, User_Input *);

#endif  // ED_CORE_H
