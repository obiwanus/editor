#include "ED_raytrace.h"

r32 Sphere::hit_by(Ray *ray) {
  // Returns the value of parameter t of the ray,
  // or -1 if no hit for positive t-s

  v3 d = ray->direction;
  v3 e = ray->origin;
  v3 c = this->center;
  r32 r = this->radius;

  v3 ec = e - c;
  r32 dd = d * d;

  // Discriminant
  r32 D = square(d * ec) - dd * (ec * ec - square(r));

  if (D < 0 || (c.w + r) >= e.w) {
    return -1;
  }

  r32 t = (-d * ec - (r32)sqrt(D)) / dd;

  return t;
}

v3 Sphere::get_normal(v3 hit_point) {
  v3 result = (hit_point - this->center).normalized();
  return result;
}

r32 Plane::hit_by(Ray *ray) {
  r32 t = ((this->point - ray->origin) * this->normal) /
          (ray->direction * this->normal);
  return t;
}

v3 Plane::get_normal(v3 hit_point) {
  // Yeah we don't need hit point I know
  return this->normal;
}

r32 Triangle::hit_by(Ray *ray) {
  return 0;
}

v3 Triangle::get_normal(v3 hit_point) {
  // Yeah yeah
  v3 result = (b - a).cross(c - a).normalized();
  return result;
}

Plane Triangle::get_plane() {
  Plane result = {};
  result.point = a;
  result.normal = (b - a).cross(c - a).normalized();
  result.color = color;
  return result;
}
