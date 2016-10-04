#ifndef __ED_RAYTRACE_H__
#define __ED_RAYTRACE_H__

#include "ED_base.h"
#include "ED_math.h"


struct Ray;

struct RayObject {
  v3 color;
  v3 specular_color = {{0.3f, 0.3f, 0.3f}};
  int phong_exp = 10;

  virtual r32 hit_by(Ray *ray) = 0;
  virtual v3 get_normal(v3 hit_point) = 0;
};

struct Sphere : RayObject {
  v3 center;
  r32 radius;

  r32 hit_by(Ray *ray) override;
  v3 get_normal(v3 hit_point) override;
};

struct Plane : RayObject {
  v3 point;
  v3 normal;

  r32 hit_by(Ray *ray) override;
  v3 get_normal(v3 hit_point) override;
};

struct Triangle : RayObject {
  v3 a, b, c;

  r32 hit_by(Ray *ray) override;
  v3 get_normal(v3 hit_point) override;
  Plane get_plane();
};

struct Ray {
  v3 origin;
  v3 direction;

  v3 point_at(r32 t) {
    v3 result = origin + direction * t;
    return result;
  }
};

struct RayHit {
  r32 at;
  RayObject *object;
};

struct RayScreen {
  int left;
  int right;
  int top;
  int bottom;

  v2i pixel_count;
};

struct LightSource {
  v3 source;
  r32 intensity;
};

#endif  // __ED_RAYTRACE_H__
