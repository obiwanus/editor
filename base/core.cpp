#include "core.h"
#include "base.h"
#include "editor_math.h"

/*
TODO:
- Allocate max size and no more glitches on resize please
- Think about the features

Features (subject to change):
- Load models
- Create models
- Edit models
- Multiple viewports/workspaces
- Ray tracer for rendering
- Interface panels, dropdowns etc
- Render fonts
- 2d editor?
-
*/

inline void DrawPixel(pixel_buffer *PixelBuffer, v2i Point, u32 Color) {
  int x = Point.x;
  int y = Point.y;

  if (x < 0 || x > PixelBuffer->width || y < 0 || y > PixelBuffer->height) {
    return;
  }
  y = PixelBuffer->height - y;  // Origin in bottom-left
  u32 *pixel = (u32 *)PixelBuffer->memory + x + y * PixelBuffer->width;
  *pixel = Color;
}

void DrawLine(pixel_buffer *PixelBuffer, v2i A, v2i B, u32 Color) {
  bool swapped = false;
  if (abs(B.x - A.x) < abs(B.y - A.y)) {
    int tmp = A.x;
    A.x = A.y;
    A.y = tmp;
    tmp = B.x;
    B.x = B.y;
    B.y = tmp;
    swapped = true;
  }
  if (B.x - A.x < 0) {
    v2i tmp = B;
    B = A;
    A = tmp;
  }

  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = dy >= 0 ? 1 : -1;
  int error = sign * dy - dx;
  int y = A.y;
  for (int x = A.x; x <= B.x; x++) {
    if (!swapped) {
      DrawPixel(PixelBuffer, {x, y}, Color);
    } else {
      DrawPixel(PixelBuffer, {y, x}, Color);
    }
    error += sign * dy;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer) {
  update_result result = {};

  // v2i A = {20, 30};
  // v2i B = {PixelBuffer->width - 20, PixelBuffer->height - 30};
  // DrawLine(PixelBuffer, A, B, 0x00FFFFFF);

  // Unit cube
  v3 points[] = {
      {-0.5f, -0.5f, 2.5f},
      {0.5f, -0.5f, 2.5f},
      {0.5f, 0.5f, 2.5f},
      {-0.5f, 0.5f, 2.5f},
      {-0.5f, -0.5f, 3.5f},
      {0.5f, -0.5f, 3.5f},
      {0.5f, 0.5f, 3.5f},
      {-0.5f, 0.5f, 3.5f},
  };

  v2i edges[] = {
      {0, 1},
      {1, 2},
      {2, 3},
      {3, 0},
      {4, 5},
      {5, 6},
      {6, 7},
      {7, 4},
      {0, 4},
      {1, 5},
      {2, 6},
      {3, 7},
  };

  // Render
  int scale = 300;
  r32 z_depth = 0;
  int x_shift = 300;
  int y_shift = 300;

  r32 angle = M_PI / 3.0f;


  // TODO: rotation



  m3x3 RotationMatrix = {
    (r32)cos(angle), -1 * (r32)sin(angle), 0,
    (r32)sin(angle), (r32)cos(angle), 0,
    0, 0, 1,
  };

  int edge_count = COUNT_OF(edges);
  for (int i = 0; i < edge_count; i++) {
    v2i edge = edges[i];
    v3 point1 = points[edge.x];
    v3 point2 = points[edge.y];
    v2 A, B;
    A.x = (point1.x * scale / (point1.z + z_depth)) + x_shift;
    A.y = (point1.y * scale / (point1.z + z_depth)) + y_shift;
    B.x = (point2.x * scale / (point2.z + z_depth)) + x_shift;
    B.y = (point2.y * scale / (point2.z + z_depth)) + y_shift;

    A = Transform(RotationMatrix, A);
    B = Transform(RotationMatrix, B);

    v2i Ai = {(int)A.x, (int)A.y};
    v2i Bi = {(int)B.x, (int)B.y};
    DrawLine(PixelBuffer, Ai, Bi, 0x00FFFFFF);
  }

  return result;
}
