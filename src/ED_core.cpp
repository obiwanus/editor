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

  // memset(PixelBuffer->memory, 0,
  //        PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  const int kSphereCount = 2;
  const int kPlaneCount = 0;
  const int kTriangleCount = 1;
  const int kRayObjCount = kSphereCount + kPlaneCount + kTriangleCount;

  const int kLightCount = 3;

  if (!gState.initialized) {
    gState.initialized = true;

    // Ray
    Ray *ray = new Ray;
    ray->origin = {0, 0, 500};

    // Spheres
    Sphere *spheres = new Sphere[kSphereCount];

    spheres[0].center = {300, 0, -500};
    spheres[0].radius = 300;
    spheres[0].color = {1.0f, 0.2f, 0.2f};
    spheres[0].phong_exp = 150;

    spheres[1].center = {-400, 100, -500};
    spheres[1].radius = 400;
    spheres[1].color = {0.2f, 0.2f, 1.0f};
    spheres[1].phong_exp = 100;

    // Planes
    // Plane *planes = new Plane[kPlaneCount];

    // planes[0].point = {0, -300, 0};
    // planes[0].normal = {0, 1, 0};
    // planes[0].normal = planes[0].normal.normalized();
    // planes[0].color = {0.1f, 0.3f, 0.2f};
    // planes[0].phong_exp = 10;

    // Triangles
    Triangle *triangles = new Triangle[kTriangleCount];

    triangles[0].a = {480, -250, -100};
    triangles[0].b = {0, 150, -700};
    triangles[0].c = {-300, -50, -10};
    triangles[0].color = {0.3f, 0.3f, 0.1f};
    triangles[0].phong_exp = 10;

    // Get a list of all objects
    RayObject **ray_objects =
        (RayObject **)malloc(kRayObjCount * sizeof(RayObject *));
    {
      RayObject **ro_pointer = ray_objects;
      for (int i = 0; i < kSphereCount; i++) {
        *ro_pointer++ = &spheres[i];
      }
      // for (int i = 0; i < kPlaneCount; i++) {
      //   *ro_pointer++ = &planes[i];
      // }
      for (int i = 0; i < kTriangleCount; i++) {
        *ro_pointer++ = &triangles[i];
      }
    }

    // Light
    LightSource *lights = new LightSource[kLightCount];

    // lights[0].intensity = 0.7f;
    // lights[0].source = {1000, 0, -500};

    lights[0].intensity = 0.7f;
    lights[0].source = {530, 200, 100};

    lights[1].intensity = 0.2f;
    lights[1].source = {-230, 100, -100};

    lights[2].intensity = 0.3f;
    lights[2].source = {-700, 400, 100};

    // Screen dimensions
    RayScreen *screen = new RayScreen;
    screen->left = -750;
    screen->right = 750;
    screen->top = 500;
    screen->bottom = -500;
    screen->x_pixel_count = 1500;
    screen->y_pixel_count = 1000;

    gState.ray = ray;
    gState.screen = screen;

    gState.spheres = spheres;
    // gState.planes = planes;
    gState.triangles = triangles;

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
                          screen->x_pixel_count,
                  screen->bottom +
                      (y + 0.5f) * (screen->top - screen->bottom) /
                          screen->y_pixel_count,
                  0};
      ray->direction = pixel - ray->origin;

      r32 min_hit = 0;
      b32 hit = false;

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      RayObject *ray_obj_hit = 0;
      for (int i = 0; i < kRayObjCount; i++) {
        RayObject *current_object = ray_objects[i];
        r32 hit_at = current_object->hit_by(ray);
        if (hit_at >= 1 && (hit_at < min_hit || min_hit == 0)) {
          hit = true;
          min_hit = hit_at;
          ray_obj_hit = current_object;
        }
      }
      if (hit) {
        v3 hit_point = ray->point_at(min_hit);
        v3 normal = ray_obj_hit->get_normal(hit_point);
        v3 line_of_sight = (pixel - hit_point).normalized();

        for (int i = 0; i < kLightCount; i++) {
          LightSource *light = &lights[i];

          v3 light_direction = (hit_point - light->source).normalized();

          b32 point_in_shadow = false;
          {
            // Cast shadow ray
            Ray shadow_ray = Ray();
            shadow_ray.origin = hit_point;
            shadow_ray.direction = -light_direction;

            // Avoid hitting itself
            const r32 kMinHitParam = 0.1f;

            for (int j = 0; j < kRayObjCount; j++) {
              RayObject *current_object = ray_objects[j];
              if (ray_obj_hit == current_object) {
                continue;
              }
              r32 hit_at = current_object->hit_by(&shadow_ray);
              if (hit_at >= kMinHitParam) {
                point_in_shadow = true;
                break;
              }
            }
          }

          if (!point_in_shadow) {
            v3 V = (-light_direction + line_of_sight).normalized();

            r32 illuminance = -light_direction * normal;
            if (illuminance < 0) {
              illuminance = 0;
            }
            color += ray_obj_hit->color * light->intensity * illuminance;

            r32 reflection = V * normal;
            if (reflection < 0) {
              reflection = 0;
            }
            v3 specular_reflection = ray_obj_hit->specular_color *
                                     light->intensity *
                                     (r32)pow(reflection, ray_obj_hit->phong_exp);
            color += specular_reflection;
          }
        }

        for (int i = 0; i < 3; i++) {
          if (color.E[i] > 1) {
            color.E[i] = 1;
          }
        }
      }
      else {
        // Background color
        color = {0.05f, 0.05f, 0.05f};
      }
      DrawPixel(PixelBuffer, {x, y}, GetRGB(color));
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
