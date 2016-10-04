#include <string.h>
#include <stdlib.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

global ProgramState gState = ProgramState();

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

u32 GetRGB(v3 Color) {
  Assert(Color.r >= 0 && Color.r <= 1);
  Assert(Color.g >= 0 && Color.g <= 1);
  Assert(Color.b >= 0 && Color.b <= 1);

  u32 result = 0x00000000;
  u8 R = (u8)(Color.r * 255);
  u8 G = (u8)(Color.g * 255);
  u8 B = (u8)(Color.b * 255);
  result = B << 16 | G << 8 | R;
  return result;
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input *Input) {
  update_result result = {};

  // memset(PixelBuffer->memory, 0,
  //        PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  if (!gState.initialized) {
    gState.initialized = true;

    // Important constants
    gState.kWindowWidth = 1000;
    gState.kWindowHeight = 700;
    gState.kMaxRecursion = 3;
    gState.kSphereCount = 3;
    gState.kPlaneCount = 1;
    gState.kTriangleCount = 0;
    gState.kRayObjCount =
        gState.kSphereCount + gState.kPlaneCount + gState.kTriangleCount;
    gState.kLightCount = 3;

    // Ray
    Ray *ray = new Ray;
    ray->origin = {0, 0, 1000};

    // Spheres
    Sphere *spheres = new Sphere[gState.kSphereCount];

    spheres[0].center = {350, 0, -1300};
    spheres[0].radius = 300;
    spheres[0].color = {0.7f, 0.7f, 0.7f};
    spheres[0].phong_exp = 10;

    spheres[1].center = {-400, 100, -1500};
    spheres[1].radius = 400;
    spheres[1].color = {0.2f, 0.2f, 0.2f};
    spheres[1].phong_exp = 500;

    spheres[2].center = {-500, -200, -1000};
    spheres[2].radius = 100;
    spheres[2].color = {0.2f, 0.2f, 0.3f};
    spheres[2].phong_exp = 1000;

    // Planes
    Plane *planes = new Plane[gState.kPlaneCount];

    planes[0].point = {0, -300, 0};
    planes[0].normal = {0, 1, 0};
    planes[0].normal = planes[0].normal.normalized();
    planes[0].color = {0.4f, 0.3f, 0.2f};
    planes[0].phong_exp = 1000;

    // Triangles
    // Triangle *triangles = new Triangle[kTriangleCount];

    // triangles[0].a = {480, -250, -100};
    // triangles[0].b = {0, 150, -700};
    // triangles[0].c = {-300, -50, -10};
    // triangles[0].color = {0.3f, 0.3f, 0.1f};
    // triangles[0].phong_exp = 10;

    // Get a list of all objects
    RayObject **ray_objects =
        (RayObject **)malloc(gState.kRayObjCount * sizeof(RayObject *));
    {
      RayObject **ro_pointer = ray_objects;
      for (int i = 0; i < gState.kSphereCount; i++) {
        *ro_pointer++ = &spheres[i];
      }
      for (int i = 0; i < gState.kPlaneCount; i++) {
        *ro_pointer++ = &planes[i];
      }
      // for (int i = 0; i < kTriangleCount; i++) {
      //   *ro_pointer++ = &triangles[i];
      // }
    }

    // Light
    LightSource *lights = new LightSource[gState.kLightCount];

    lights[0].intensity = 0.7f;
    lights[0].source = {1730, 600, -200};

    lights[1].intensity = 0.4f;
    lights[1].source = {-300, 1000, -100};

    lights[2].intensity = 0.4f;
    lights[2].source = {-1700, 300, 100};

    // Screen dimensions
    RayScreen *screen = new RayScreen;
    screen->pixel_count = {gState.kWindowWidth, gState.kWindowHeight};
    screen->left = -screen->pixel_count.x / 2;
    screen->right = screen->pixel_count.x / 2;
    screen->bottom = -screen->pixel_count.y / 2;
    screen->top = screen->pixel_count.y / 2;

    gState.ray = ray;
    gState.screen = screen;

    gState.spheres = spheres;
    gState.planes = planes;
    // gState.triangles = triangles;

    gState.ray_objects = ray_objects;

    gState.lights = lights;
  }

  Ray *ray = gState.ray;
  RayScreen *screen = gState.screen;
  LightSource *lights = gState.lights;
  RayObject **ray_objects = gState.ray_objects;

  if (Input->up) {
    gState.lights[0].source.v += 100;
  }
  if (Input->down) {
    gState.lights[0].source.v -= 100;
  }
  if (Input->left) {
    gState.lights[0].source.u -= 100;
  }
  if (Input->right) {
    gState.lights[0].source.u += 100;
  }

  for (int x = 0; x < PixelBuffer->width; x++) {
    for (int y = 0; y < PixelBuffer->height; y++) {
      // Get the ray
      v3 pixel = {screen->left +
                      (x + 0.5f) * (screen->right - screen->left) /
                          screen->pixel_count.x,
                  screen->bottom +
                      (y + 0.5f) * (screen->top - screen->bottom) /
                          screen->pixel_count.y,
                  0};
      ray->direction = pixel - ray->origin;

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      color += GetRayColor(&gState, ray, 0, gState.kMaxRecursion);

      // Crop
      for (int i = 0; i < 3; i++) {
        if (color.E[i] > 1) {
          color.E[i] = 1;
        }
        if (color.E[i] < 0) {
          color.E[i] = 0;
        }
      }

      DrawPixel(PixelBuffer, {x, y}, GetRGB(color));
    }
  }

  return result;
}
