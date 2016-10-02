#include <string.h>
#include <stdlib.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

global program_state gState = program_state();

global const int kWindowWidth = 1500;
global const int kWindowHeight = 1000;

global const int kMaxRecursion = 10;

global const int kSphereCount = 3;
global const int kPlaneCount = 1;
global const int kTriangleCount = 0;
global const int kRayObjCount = kSphereCount + kPlaneCount + kTriangleCount;

global const int kLightCount = 3;

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

v3 GetRayColor(Ray *ray, RayObject *reflected_from, int recurse_further) {
  v3 color = {};

  r32 min_hit = 0;
  b32 hit = false;
  RayObject *ray_obj_hit = 0;
  for (int i = 0; i < kRayObjCount; i++) {
    RayObject *current_object = gState.ray_objects[i];
    if (current_object == reflected_from) {
      // NOTE: won't work for complex shapes
      continue;
    }
    r32 hit_at = current_object->hit_by(ray);
    if (hit_at >= 0 && (hit_at < min_hit || min_hit == 0)) {
      hit = true;
      min_hit = hit_at;
      ray_obj_hit = current_object;
    }
  }
  if (hit) {
    v3 hit_point = ray->point_at(min_hit);
    v3 normal = ray_obj_hit->get_normal(hit_point);
    v3 line_of_sight = -ray->direction.normalized();

    for (int i = 0; i < kLightCount; i++) {
      LightSource *light = &gState.lights[i];

      v3 light_direction = (hit_point - light->source).normalized();

      b32 point_in_shadow = false;
      {
        // Cast shadow ray
        Ray shadow_ray = Ray();
        shadow_ray.origin = hit_point;
        shadow_ray.direction =
            light->source - hit_point;  // not normalizing on purpose

        for (int j = 0; j < kRayObjCount; j++) {
          RayObject *current_object = gState.ray_objects[j];
          if (ray_obj_hit == current_object) {
            continue;
          }
          r32 hit_at = current_object->hit_by(&shadow_ray);
          if (hit_at >= 0 && hit_at <= 1) {
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

        // Calculate specular reflection
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

    // Calculate mirror reflection
    if (recurse_further) {
      Ray reflection_ray = {};
      reflection_ray.origin = hit_point;
      reflection_ray.direction =
          ray->direction - 2 * (ray->direction * normal) * normal;

      const r32 max_k = 0.3f;
      r32 k = ray_obj_hit->phong_exp * 0.001f;
      if (k > max_k) {
        k = max_k;
      }
      color +=
          k * GetRayColor(&reflection_ray, ray_obj_hit, recurse_further - 1);
      // color += GetRayColor(&reflection_ray, recurse_further - 1);
    }
  } else if (recurse_further == kMaxRecursion) {
    // Background color
    color = {0.05f, 0.05f, 0.05f};
  }
  return color;
}

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input *Input) {
  update_result result = {};

  // memset(PixelBuffer->memory, 0,
  //        PixelBuffer->height * PixelBuffer->width * sizeof(u32));

  if (!gState.initialized) {
    gState.initialized = true;

    // Ray
    Ray *ray = new Ray;
    ray->origin = {0, 0, 1000};

    // Spheres
    Sphere *spheres = new Sphere[kSphereCount];

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
    Plane *planes = new Plane[kPlaneCount];

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
        (RayObject **)malloc(kRayObjCount * sizeof(RayObject *));
    {
      RayObject **ro_pointer = ray_objects;
      for (int i = 0; i < kSphereCount; i++) {
        *ro_pointer++ = &spheres[i];
      }
      for (int i = 0; i < kPlaneCount; i++) {
        *ro_pointer++ = &planes[i];
      }
      // for (int i = 0; i < kTriangleCount; i++) {
      //   *ro_pointer++ = &triangles[i];
      // }
    }

    // Light
    LightSource *lights = new LightSource[kLightCount];

    lights[0].intensity = 0.7f;
    lights[0].source = {1730, 600, -200};

    lights[1].intensity = 0.4f;
    lights[1].source = {-300, 1000, -100};

    lights[2].intensity = 0.4f;
    lights[2].source = {-1700, 300, 100};

    // Screen dimensions
    RayScreen *screen = new RayScreen;
    screen->pixel_count = {kWindowWidth, kWindowHeight};
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

      color += GetRayColor(ray, 0, kMaxRecursion);

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
