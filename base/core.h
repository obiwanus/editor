#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

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

update_result UpdateAndRender(pixel_buffer *PixelBuffer);

#endif  // EDITOR_CORE_H