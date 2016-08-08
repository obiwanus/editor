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
  v3 angle;
  v2 drag_start = {-1, -1};
  v2 drag_current;
  v2 base = {300.0f, 300.0f};
  v2 pointer;
  int scale = 300;
};

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input Input);

#endif  // EDITOR_CORE_H