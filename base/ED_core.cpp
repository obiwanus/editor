#include <string.h>
#include <stdlib.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"


global program_state gState = program_state();


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

inline void DrawPixelV2(pixel_buffer *PixelBuffer, v2 Point, u32 Color) {
  // A v2 version
  v2i point = {(int)Point.x, (int)Point.y};
  DrawPixel(PixelBuffer, point, Color);
}

// TODO: draw triangles
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

void DrawLine(pixel_buffer *PixelBuffer, v2 A, v2 B, u32 Color) {
  v2i a = {(int)A.x, (int)A.y};
  v2i b = {(int)B.x, (int)B.y};
  DrawLine(PixelBuffer, a, b, Color);
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input *Input) {
  update_result result = {};

  memset(PixelBuffer->memory, 0,
         PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  // Ray casting
  v3 e = gState.e;
  v3 d = {0, 0, -1};

  if (Input->up) {
    gState.e.z += 1;
  } else if (Input->down && gState.e.z > 0) {
    gState.e.z -= 1;
  }

  // Sphere
  v3 c = {0, 0, -50};
  r32 R = 10;

  // Light direction
  v3 light = {-1, 1, 0};

  // Screen dimensions
  int l = -20, r = 20, t = 15, b = -15;
  int nx = 400;
  int ny = 300;

  u32 AMBIENT = 0x00333333;

  for (int x = 0; x < PixelBuffer->width; x++) {
    for (int y = 0; y < PixelBuffer->height; y++) {
      // Get the ray
      v3 p = {l + (r32)(x + 0.5f) * (r32)(r - l) / (r32)nx, b + (r32)(y + 0.5f) * (t - b) / (r32)ny, 0};
      d = p - e;

      // Discriminant
      r32 D = square(d * (e - c)) - (d * d) * ((e - c) * (e - c) - square(R));
      if (D >= 0 && c.z < e.z) {
        // Find the point of intersection
        // we're only interested in the closest one
        r32 param = (-d * (e - c) - (r32)sqrt(D)) / (d * d);
        if (param > 0) {
          v3 point = e + d * param;
          v3 normal = point - c;

          r32 brightness = normal * light / 10.0f;
          u8 component = (u8)(brightness * 100);
          u32 color = component | component << 8 | component << 16;
          DrawPixel(PixelBuffer, {x, y}, color);
        }
      } else {
        // Ray misses the sphere
        DrawPixel(PixelBuffer, {x, y}, AMBIENT);
      }
    }
  }

  // if (Input->up) {
  //   gState.scale += 10;
  // } else if (Input->down) {
  //   gState.scale -= 10;
  // }

  // if (Input->left) {
  //   gState.angle.y += 0.05f;
  // } else if (Input->right) {
  //   gState.angle.y -= 0.05f;
  // }

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

  v3 center = V3(0, 0, 3.0f);

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
  // r32 z_depth = 0;

  // int scale = gState.scale;
  // v2 base = gState.base;
  // v3 angle = gState.angle;

  // // quaternions?
  // m3x3 RotationMatrixX = {
  //     1, 0, 0, 0, (r32)cos(angle.x), -1 * (r32)sin(angle.x), 0,
  //     (r32)sin(angle.x), (r32)cos(angle.x),
  // };

  // m3x3 RotationMatrixY = {
  //     (r32)cos(angle.y), 0, -1 * (r32)sin(angle.y), 0, 1, 0, (r32)sin(angle.y),
  //     0, (r32)cos(angle.y),
  // };

  // m3x3 RotationMatrixZ = {
  //     (r32)cos(angle.z), -1 * (r32)sin(angle.z), 0, (r32)sin(angle.z),
  //     (r32)cos(angle.z), 0, 0, 0, 1,
  // };

  // int edge_count = COUNT_OF(edges);
  // for (int i = 0; i < edge_count; i++) {
  //   v2i edge = edges[i];
  //   v3 point1 = points[edge.x];
  //   v3 point2 = points[edge.y];

  //   point1 = Rotate(RotationMatrixY, point1, center);
  //   point2 = Rotate(RotationMatrixY, point2, center);

  //   point1 = Rotate(RotationMatrixX, point1, center);
  //   point2 = Rotate(RotationMatrixX, point2, center);

  //   point1 = Rotate(RotationMatrixZ, point1, center);
  //   point2 = Rotate(RotationMatrixZ, point2, center);

  //   v2 A, B;
  //   A.x = (point1.x * scale / (point1.z + z_depth)) + base.x;
  //   A.y = (point1.y * scale / (point1.z + z_depth)) + base.y;
  //   B.x = (point2.x * scale / (point2.z + z_depth)) + base.x;
  //   B.y = (point2.y * scale / (point2.z + z_depth)) + base.y;

  //   v2i Ai = {(int)A.x, (int)A.y};
  //   v2i Bi = {(int)B.x, (int)B.y};
  //   DrawLine(PixelBuffer, Ai, Bi, 0x00FFFFFF);
  // }

  // if (Input->mouse_middle) {
  //   DrawLine(PixelBuffer, base, {Input->mouse.x, Input->mouse.y}, 0x00FFFFFF);
  // }

  return result;
}
