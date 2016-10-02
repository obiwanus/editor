#include "ED_raytrace.h"


r32 Sphere::hit_by(Ray *ray) {
  // Returns the value of parameter t of the ray,
  // or -1 if no hit for positive t-s
  r32 t = -1;

  v3 d = ray->direction;
  v3 e = ray->origin;
  v3 c = this->center;
  r32 r = this->radius;

  v3 ec = e - c;
  r32 dd = d * d;

  // Discriminant
  r32 D = square(d * ec) - dd * (ec * ec - square(r));

  if (D < 0) {
    return -1;
  }

  t = (-d * ec - (r32)sqrt(D)) / dd;

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
  m3x3 A;
  A.rows[0] = {a.x - b.x, a.x - c.x, ray->direction.x};
  A.rows[1] = {a.y - b.y, a.y - c.y, ray->direction.y};
  A.rows[2] = {a.z - b.z, a.z - c.z, ray->direction.z};

  r32 D = A.determinant();
  if (D == 0) {
    return -1;
  }

  v3 R = {a.x - ray->origin.x, a.y - ray->origin.y, a.z - ray->origin.z};

  // Use Cramer's rule to find t, beta, and gamma
  m3x3 A2 = A.replace_column(2, R);
  r32 t = A2.determinant() / D;
  if (t <= 0) {
    return -1;
  }

  r32 gamma = A.replace_column(1, R).determinant() / D;
  if (gamma < 0 || gamma > 1) {
    return -1;
  }

  r32 beta = A.replace_column(0, R).determinant() / D;
  if (beta < 0 || beta > (1 - gamma)) {
    return -1;
  }

  return t;
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
