#include <string.h>
#include <stdlib.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

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

  // TODO:
  // Init stuff only once

  // memset(PixelBuffer->memory, 0,
  //        PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  const int SPHERE_COUNT = 2;
  const int PLANE_COUNT = 1;
  const int TRI_COUNT = 1;
  const int RAY_OBJ_COUNT = SPHERE_COUNT + PLANE_COUNT + TRI_COUNT;

  if (!gState.initialized) {
    gState.initialized = true;

    // Ray
    Ray *ray = (Ray *)calloc(1, sizeof(Ray));
    *ray = {};
    ray->origin = {0, 0, 50};

    // Spheres
    Sphere *spheres = (Sphere *)calloc(SPHERE_COUNT, sizeof(Sphere));

    spheres[0].center = {10, -5, -45};
    spheres[0].radius = 15;
    spheres[0].color = {1.0f, 0.2f, 0.2f};
    spheres[0].phong_exp = 100;

    spheres[1].center = {-10, 0, -50};
    spheres[1].radius = 20;
    spheres[1].color = {0.2f, 0.2f, 1.0f};
    spheres[1].phong_exp = 50;

    // Planes
    Plane *planes = (Plane *)calloc(PLANE_COUNT, sizeof(Plane));

    planes[0].point = {0, -20, 0};
    planes[0].normal = {0, 1, 0};
    planes[0].normal = planes[0].normal.normalized();
    planes[0].color = {0.2f, 0.7f, 0.2f};

    // Triangles
    Triangle *triangles = (Triangle *)calloc(TRI_COUNT, sizeof(Triangle));

    triangles[0].a = {-10, 0, -50};
    triangles[0].c = {10, 0, -50};
    triangles[0].b = {0, 0, 0};
    triangles[0].color = {0.3f, 0.3f, 0.3f};






    // TODO:
    // Find out how you're supposed to create objects
    // so that they dont lose they virtual function pointers







    // Get a list of all objects
    // RayObject **ray_objects =
    //     (RayObject **)calloc(RAY_OBJ_COUNT, sizeof(RayObject *));
    // {
    //   RayObject **ro_pointer = ray_objects;
    //   for (int i = 0; i < SPHERE_COUNT; i++) {
    //     *ro_pointer++ = &spheres[i];
    //   }
    //   for (int i = 0; i < PLANE_COUNT; i++) {
    //     *ro_pointer++ = &planes[i];
    //   }
    //   for (int i = 0; i < TRI_COUNT; i++) {
    //     *ro_pointer++ = &triangles[i];
    //   }
    // }

    // Light
    LightSource *light = (LightSource *)calloc(1, sizeof(LightSource));
    light->intensity = 0.5f;
    light->source = {130, 0, 100};

    // Screen dimensions
    RayScreen *screen = (RayScreen *)calloc(1, sizeof(RayScreen));
    screen->left = -20;
    screen->right = 20;
    screen->top = 15;
    screen->bottom = -15;
    screen->x_pixel_count = 400;
    screen->y_pixel_count = 300;

    gState.ray = ray;
    gState.screen = screen;

    gState.spheres = spheres;
    gState.planes = planes;
    gState.triangles = triangles;

    gState.ray_objects = ray_objects;

    gState.light = light;
  }

  Ray *ray = gState.ray;
  RayScreen *screen = gState.screen;
  LightSource *light = gState.light;

  // Stack
  RayObject *ray_objects[RAY_OBJ_COUNT];
  {
    RayObject **ro_pointer = ray_objects;
    for (int i = 0; i < SPHERE_COUNT; i++) {
      *ro_pointer++ = &gState.spheres[i];
    }
    for (int i = 0; i < PLANE_COUNT; i++) {
      *ro_pointer++ = &gState.planes[i];
    }
    for (int i = 0; i < TRI_COUNT; i++) {
      *ro_pointer++ = &gState.triangles[i];
    }
  }

  if (Input->up) {
    gState.light->source.v += 10;
  }
  if (Input->down) {
    gState.light->source.v -= 10;
  }
  if (Input->left) {
    gState.light->source.u -= 10;
  }
  if (Input->right) {
    gState.light->source.u += 10;
  }

  for (int x = 0; x < PixelBuffer->width; x++) {
    for (int y = 0; y < PixelBuffer->height; y++) {
      // Get the ray
      v3 pixel = {screen->left +
                      (x + 0.5f) * (screen->right - screen->left) /
                          screen->x_pixel_count,
                  screen->bottom +
                      (y + 0.5f) * (screen->top - screen->bottom) /
                          screen->y_pixel_count,
                  0};
      ray->direction = pixel - ray->origin;

      r32 min_hit = 0;
      b32 hit = false;

      RayObject *ray_obj_hit = 0;
      for (int i = 0; i < RAY_OBJ_COUNT; i++) {
        RayObject *current_object = ray_objects[i];
        r32 hit_at = current_object->hit_by(ray);
        if (hit_at > 0 && (hit_at < min_hit || min_hit == 0)) {
          hit = true;
          min_hit = hit_at;
          ray_obj_hit = current_object;
        }
      }
      if (hit) {
        v3 hit_point = ray->point_at(min_hit);
        v3 normal = ray_obj_hit->get_normal(hit_point);
        v3 light_direction = (hit_point - light->source).normalized();
        v3 line_of_sight = (pixel - hit_point).normalized();

        v3 V = (-light_direction + line_of_sight).normalized();

        r32 illuminance = -light_direction * normal;
        if (illuminance < 0) {
          illuminance = 0;
        }
        v3 color = ray_obj_hit->color * light->intensity * illuminance;

        r32 reflection = V * normal;
        if (reflection < 0) {
          reflection = 0;
        }
        v3 specular_reflection = ray_obj_hit->specular_color *
                                 light->intensity *
                                 (r32)pow(reflection, ray_obj_hit->phong_exp);
        color += specular_reflection;
        for (int i = 0; i < 3; i++) {
          if (color.e[i] > 1) {
            color.e[i] = 1;
          }
        }
        DrawPixel(PixelBuffer, {x, y}, GetRGB(color));
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
  //     (r32)cos(angle.y), 0, -1 * (r32)sin(angle.y), 0, 1, 0,
  //     (r32)sin(angle.y),
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
  //   DrawLine(PixelBuffer, base, {Input->mouse.x, Input->mouse.y},
  //   0x00FFFFFF);
  // }

  return result;
}