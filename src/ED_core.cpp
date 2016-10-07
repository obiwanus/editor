#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

global ProgramState gState = ProgramState();

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2i Point, u32 Color) {
  int x = Point.x;
  int y = Point.y;

  if (x < 0 || x > pixel_buffer->width || y < 0 || y > pixel_buffer->height) {
    return;
  }
  y = pixel_buffer->height - y;  // Origin in bottom-left
  u32 *pixel = (u32 *)pixel_buffer->memory + x + y * pixel_buffer->width;
  *pixel = Color;
}

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2 Point, u32 Color) {
  // A v2 version
  v2i point = {(int)Point.x, (int)Point.y};
  draw_pixel(pixel_buffer, point, Color);
}

// TODO: draw triangles
void draw_line(Pixel_Buffer *pixel_buffer, v2i A, v2i B, u32 Color) {
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
      draw_pixel(pixel_buffer, V2i(x, y), Color);
    } else {
      draw_pixel(pixel_buffer, V2i(y, x), Color);
    }
    error += sign * dy;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_line(Pixel_Buffer *pixel_buffer, v2 A, v2 B, u32 Color) {
  v2i a = {(int)A.x, (int)A.y};
  v2i b = {(int)B.x, (int)B.y};
  draw_line(pixel_buffer, a, b, Color);
}

inline u32 GetRGB(v3 Color) {
  Assert(Color.r >= 0 && Color.r <= 1);
  Assert(Color.g >= 0 && Color.g <= 1);
  Assert(Color.b >= 0 && Color.b <= 1);

  u32 result = 0x00000000;
  u8 R = (u8)(Color.r * 255);
  u8 G = (u8)(Color.g * 255);
  u8 B = (u8)(Color.b * 255);
  result = R << 16 | G << 8 | B;
  return result;
}

void draw_rect(Pixel_Buffer *pixel_buffer, Rect rect, v3 color) {
  u32 rgb = GetRGB(color);

  if (rect.left < 0) rect.left = 0;
  if (rect.bottom < 0) rect.bottom = 0;
  if (rect.right > pixel_buffer->width) rect.right = pixel_buffer->width;
  if (rect.top > pixel_buffer->height) rect.top = pixel_buffer->height;

  for (int x = rect.left; x < rect.right; x++) {
    for (int y = rect.bottom; y < rect.top; y++) {
      // Don't care about performance
      draw_pixel(pixel_buffer, V2i(x, y), rgb);
    }
  }
}

Area::Area(v2i p1, v2i p2, v3 color) {
  this->rect.left = p1.x < p2.x ? p1.x : p2.x;
  this->rect.right = p1.x < p2.x ? p2.x : p1.x;
  this->rect.bottom = p1.y < p2.y ? p1.y : p2.y;
  this->rect.top = p1.y < p2.y ? p2.y : p1.y;

  this->color = color;
}

void Area::draw(Pixel_Buffer *pixel_buffer) {
  draw_rect(pixel_buffer, this->rect, this->color);
}

// TODO: don't make gState global?

update_result UpdateAndRender(Pixel_Buffer *pixel_buffer, user_input *Input) {
  update_result result = {};

  // Clear
  memset(pixel_buffer->memory, 0,
         pixel_buffer->width * pixel_buffer->height * 4);

  if (!gState.initialized) {
    gState.initialized = true;
    gState.panel1 = Area({10, 10}, {500, 500}, V3(0.1f, 0.2f, 0.3f));
  }

  Area *Area = &gState.panel1;



#if 0

  RayCamera *camera = &gState.camera;

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

    // Camera dimensions
    camera->origin = {0, 0, 1000};
    camera->pixel_count = {gState.kWindowWidth, gState.kWindowHeight};
    camera->left = -camera->pixel_count.x / 2;
    camera->right = camera->pixel_count.x / 2;
    camera->bottom = -camera->pixel_count.y / 2;
    camera->top = camera->pixel_count.y / 2;

    gState.spheres = spheres;
    gState.planes = planes;
    // gState.triangles = triangles;

    gState.ray_objects = ray_objects;

    gState.lights = lights;
  }

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

  if (Input->mouse_left) {
    Ray pointer_ray =
        camera->get_ray_through_pixel((int)Input->mouse.x, (int)Input->mouse.y);

    RayHit pointer_hit =
        pointer_ray.get_object_hit(&gState, 0, INFINITY, NULL);
    if (pointer_hit.object != NULL) {
      pointer_hit.object->color.x += 0.1f;
    }
  }

  for (int x = 0; x < pixel_buffer->width; x++) {
    for (int y = 0; y < pixel_buffer->height; y++) {
      Ray ray = camera->get_ray_through_pixel(x, y);

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      color += ray.get_color(&gState, 0, gState.kMaxRecursion);

      // Crop
      for (int i = 0; i < 3; i++) {
        if (color.E[i] > 1) {
          color.E[i] = 1;
        }
        if (color.E[i] < 0) {
          color.E[i] = 0;
        }
      }

      draw_pixel(pixel_buffer, V2i(x, y), GetRGB(color));
    }
  }

#endif  // if 0

  return result;
}
