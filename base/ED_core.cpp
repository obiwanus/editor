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

struct Sphere {
  v3 center;
  r32 radius;
  u32 color;
};

struct Ray {
  v3 origin;
  v3 direction;
};

r32 intersect(Ray ray, Sphere sphere) {
  v3 d = ray.direction;
  v3 e = ray.origin;
  v3 c = sphere.center;
  r32 r = sphere.radius;

  // Discriminant
  r32 D = square(d * (e - c)) - (d * d) * ((e - c) * (e - c) - square(r));

  if (D < 0 || (c.w + r) >= e.w) {
    return -1;
  }

  r32 param = (-d * (e - c) - (r32)sqrt(D)) / (d * d);

  return param;
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input *Input) {
  update_result result = {};

  memset(PixelBuffer->memory, 0,
         PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  if (Input->up) {
    gState.point.v += 1;
  }
  if (Input->down) {
    gState.point.v -= 1;
  }
  if (Input->left) {
    gState.point.w += 1;
  }
  if (Input->right) {
    gState.point.w -= 1;
  }

  // Ray casting
  Ray ray = {};
  ray.origin = {0, 0, 50};

  // Sphere
  const int SPHERE_COUNT = 2;
  Sphere spheres[SPHERE_COUNT];

  spheres[0].center = {10, 0, -50};
  spheres[0].radius = 20;
  spheres[0].color = 0x00FF3333;

  spheres[1].center = gState.point;
  spheres[1].radius = 20;
  spheres[1].color = 0x003333FF;

  // Light direction
  v3 light = {-1, -1, 1};

  // Screen dimensions
  int l = -20, r = 20, t = 15, b = -15;
  int nx = 400;
  int ny = 300;

  for (int x = 0; x < PixelBuffer->width; x++) {
    for (int y = 0; y < PixelBuffer->height; y++) {
      // Get the ray
      v3 pixel = {l + (x + 0.5f) * (r - l) / nx, b + (y + 0.5f) * (t - b) / ny, 0};
      ray.direction = pixel - ray.origin;

      r32 min_hit = 0;
      b32 hit = false;
      Sphere hit_sphere = {};
      for (int i = 0; i < SPHERE_COUNT; i++) {;
        r32 hit_at = intersect(ray, spheres[i]);
        if (hit_at > 0 && (hit_at < min_hit || min_hit == 0)) {
          hit = true;
          min_hit = hit_at;
          hit_sphere = spheres[i];
        }
      }
      if (hit) {
        DrawPixel(PixelBuffer, {x, y}, hit_sphere.color);
      } else {
        DrawPixel(PixelBuffer, {x, y}, 0x00333333);  // ambient
      }
    }
  }

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
