#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

#include "base.h"
#include "editor_math.h"

struct pixel_buffer {
  int width;
  int height;
  int max_width;
  int max_height;
  void *memory;
};

struct update_result {
  // empty for now
};

struct user_input {
  r32 angle;
  v2 base;
  v2 pointer;
};

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input Input);

#endif  // EDITOR_CORE_H