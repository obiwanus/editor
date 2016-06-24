#include "core.h"
#include "base.h"

// NOTE: temporary function
inline void DrawPixel(pixel_buffer *PixelBuffer, int X, int Y, u32 Color) {
  if (X < 0 || X > PixelBuffer->width || Y < 0 || Y > PixelBuffer->height) {
    return;
  }
  u32 *pixel = (u32 *)PixelBuffer->memory + X + Y * PixelBuffer->width;
  *pixel = Color;
}

struct v2i {
  int x;
  int y;
};

struct v3f {
  r32 x;
  r32 y;
  r32 z;
};

// NOTE: temporary function
void DrawLine(pixel_buffer *PixelBuffer, v2i A, v2i B, u32 Color) {
  v2i left = A;
  v2i right = B;

  if (A.x == B.x) {
    int step = (A.y < B.y) ? 1 : -1;
    int y = A.y;
    while (y != B.y) {
      DrawPixel(PixelBuffer, A.x, y, Color);
      y += step;
    }
    return;
  }

  if (A.x > B.x) {
    left = B;
    right = A;
  }

  for (int x = left.x; x < right.x; x++) {
    r32 t = (x - left.x) / (r32)(right.x - left.x);
    int y = (int)(left.y * (1.0f - t) + right.y * t);
    DrawPixel(PixelBuffer, x, y, Color);
  }
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer) {
  update_result result = {};

  // v2i A = {20, 30};
  // v2i B = {PixelBuffer->width - 20, PixelBuffer->height - 30};
  // DrawLine(PixelBuffer, A, B, 0x00FFFFFF);

  // Unit cube
  v3f points[] = {
      {-0.5f, -0.5f, -0.5f},
      {0.5f, -0.5f, -0.5f},
      {0.5f, 0.5f, -0.5f},
      {-0.5f, 0.5f, -0.5f},
      {-0.5f, -0.5f, 0.5f},
      {0.5f, -0.5f, 0.5f},
      {0.5f, 0.5f, 0.5f},
      {-0.5f, 0.5f, 0.5f},
  };

  v2i edges[] = {
      {0, 1}, {1, 2}, {2, 3}, {3, 0},
      {4, 5}, {5, 6}, {6, 7}, {7, 4},
      {0, 4}, {1, 5}, {2, 6}, {3, 7},
  };

  // Render
  int scale = 300;
  r32 z_depth = 1.0f;
  int x_shift = 10;
  int y_shift = 10;

  int edge_count = COUNT_OF(edges);
  for (int i = 0; i < edge_count; i++) {
    v2i edge = edges[i];
    v3f point1 = points[edge.x];
    v3f point2 = points[edge.y];
    v2i A, B;
    A.x = (int)(point1.x * scale / (point1.z + z_depth)) + x_shift + scale;
    A.y = (int)(point1.y * scale / (point1.z + z_depth)) + y_shift + scale;
    B.x = (int)(point2.x * scale / (point2.z + z_depth)) + x_shift + scale;
    B.y = (int)(point2.y * scale / (point2.z + z_depth)) + y_shift + scale;
    DrawLine(PixelBuffer, A, B, 0x00FFFFFF);
  }

  return result;
}
