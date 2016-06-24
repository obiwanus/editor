#include "core.h"
#include "base.h"

// NOTE: temporary function
void DrawPixel(pixel_buffer *PixelBuffer, int X, int Y, u32 Color) {
  u32 *pixel = (u32 *)PixelBuffer->memory + X + Y * PixelBuffer->width;
  *pixel = Color;
}

struct point {
  int x;
  int y;
};

// NOTE: temporary function
void DrawLine(pixel_buffer *PixelBuffer, point A, point B, u32 Color) {
  point left = A;
  point right = B;

  Assert(A.x != B.x);

  if (A.x > B.x) {
    left = A;
    right = B;
  }

  for (int x = left.x; x < right.x; x++) {
    r32 dy = ((r32)(x - left.x) / (r32)(right.x - left.x)) * (right.y - left.y);
    int y = left.y + (int)dy;
    DrawPixel(PixelBuffer, x, y, Color);
  }
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer) {
  update_result result = {};

  point A = {20, 30};
  point B = {PixelBuffer->width - 20, PixelBuffer->height - 30};
  DrawLine(PixelBuffer, A, B, 0x00FFFFFF);

  return result;
}
