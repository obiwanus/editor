#ifndef __ED_RAYTRACE_H__
#define __ED_RAYTRACE_H__

#include "ED_base.h"
#include "ED_math.h"

struct Ray;
struct Program_State;

typedef enum {
  RayObject_Type_Invalid = 0,
  RayObject_Type_Sphere,
  RayObject_Type_Plane,
  RayObject_Type_Triangle,
} RayObject_Type;

struct RayObject {
  v3 color;
  v3 specular_color = {{0.3f, 0.3f, 0.3f}};
  int phong_exp = 10;

  virtual r32 hit_by(Ray *ray) = 0;
  virtual v3 get_normal(v3 hit_point) = 0;
  virtual RayObject_Type get_type() { return RayObject_Type_Invalid; }
};

struct Sphere : RayObject {
  v3 center;
  r32 radius;

  r32 hit_by(Ray *ray) override;
  v3 get_normal(v3 hit_point) override;
  RayObject_Type get_type() override { return RayObject_Type_Sphere; }
};

struct Plane : RayObject {
  v3 point;
  v3 normal;

  r32 hit_by(Ray *ray) override;
  v3 get_normal(v3 hit_point) override;
  RayObject_Type get_type() override { return RayObject_Type_Plane; }
};

// struct Triangle : RayObject {
//   v3 a, b, c;

//   r32 hit_by(Ray *ray) override;
//   v3 get_normal(v3 hit_point) override;
//   RayObject_Type get_type() override { return RayObject_Type_Triangle; }
//   Plane get_plane();
// };

struct RayHit {
  r32 at;
  RayObject *object;
};

struct Ray {
  v3 origin;
  v3 direction;

  v3 point_at(r32 t) {
    v3 result = origin + direction * t;
    return result;
  }

  v3 get_color(Program_State *state, RayObject *reflected_from,
               int recurse_further);
  RayHit get_object_hit(Program_State *state, r32 tmin, r32 tmax,
                        RayObject *ignore_object = NULL, b32 any = false);
};

struct RayCamera {
  int left;
  int right;
  int top;
  int bottom;

  v2i pixel_count;
  v3 origin;

  Ray get_ray_through_pixel(int x, int y);
};

struct LightSource {
  v3 source;
  r32 intensity;
};

#endif  // __ED_RAYTRACE_H__
