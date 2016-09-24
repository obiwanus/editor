#ifndef __ED_RAYTRACE_H__
#define __ED_RAYTRACE_H__

#include "ED_base.h"
#include "ED_math.h"


struct Ray;

struct Object {
  v3 color;
  v3 specular_color = {{0.3f, 0.3f, 0.3f}};
  int phong_exp = 10;

  virtual r32 hit_by(Ray *ray);
};

struct Sphere : Object {
  v3 center;
  r32 radius;

  r32 hit_by(Ray *ray) override;
};

struct Plane : Object {
  v3 point;
  v3 normal;
  v3 color;

  r32 hit_by(Ray *ray) override;
};

struct Ray {
  v3 origin;
  v3 direction;

  v3 point_at(r32 t) {
    v3 result = origin + direction * t;
    return result;
  }
};

struct LightSource {
  v3 source;
  r32 intensity;
};

struct Triangle : Object {
  v3 a, b, c;

  r32 hit_by(Ray *ray) override;
  Plane get_plane();
};

#endif  // __ED_RAYTRACE_H__
