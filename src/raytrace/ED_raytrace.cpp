#include <stdio.h>

#include "ED_raytrace.h"
#include "ED_core.h"

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

// r32 Triangle::hit_by(Ray *ray) {
//   m3x3 A;
//   A.rows[0] = {a.x - b.x, a.x - c.x, ray->direction.x};
//   A.rows[1] = {a.y - b.y, a.y - c.y, ray->direction.y};
//   A.rows[2] = {a.z - b.z, a.z - c.z, ray->direction.z};

//   r32 D = A.determinant();
//   if (D == 0) {
//     return -1;
//   }

//   v3 R = {a.x - ray->origin.x, a.y - ray->origin.y, a.z - ray->origin.z};

//   // Use Cramer's rule to find t, beta, and gamma
//   m3x3 A2 = A.replace_column(2, R);
//   r32 t = A2.determinant() / D;
//   if (t <= 0) {
//     return -1;
//   }

//   r32 gamma = A.replace_column(1, R).determinant() / D;
//   if (gamma < 0 || gamma > 1) {
//     return -1;
//   }

//   r32 beta = A.replace_column(0, R).determinant() / D;
//   if (beta < 0 || beta > (1 - gamma)) {
//     return -1;
//   }

//   return t;
// }

// v3 Triangle::get_normal(v3 hit_point) {
//   // Yeah yeah
//   v3 result = (b - a).cross(c - a).normalized();
//   return result;
// }

// Plane Triangle::get_plane() {
//   Plane result = {};
//   result.point = a;
//   result.normal = (b - a).cross(c - a).normalized();
//   result.color = color;
//   return result;
// }

Ray RayCamera::get_ray_through_pixel(int x, int y, v2i pixel_count) {
  Ray result;

  v3 pixel = {left + (x + 0.5f) * (right - left) / pixel_count.x,
              bottom + (y + 0.5f) * (top - bottom) / pixel_count.y, 0};

  result.origin = origin;
  result.direction = pixel - origin;

  return result;
}

RayHit Ray::get_object_hit(Program_State *state, r32 tmin, r32 tmax,
                           RayObject *ignore_object, b32 any) {
  Ray *ray = this;

  RayHit ray_hit = {};
  r32 min_hit = 0;

  for (int i = 0; i < state->kRayObjCount; i++) {
    RayObject *current_object = state->ray_objects[i];
    if (current_object == ignore_object) {
      // NOTE: won't work for reflections on non-convex shapes
      continue;
    }
    r32 hit_at = current_object->hit_by(ray);
    if (tmin <= hit_at && hit_at <= tmax &&
        (hit_at < min_hit || min_hit == 0)) {
      ray_hit.object = current_object;
      ray_hit.at = hit_at;
      min_hit = hit_at;

      if (any) {
        return ray_hit;  // don't care about the closest one
      }
    }
  }

  return ray_hit;
}

v3 Ray::get_color(Program_State *state, RayObject *reflected_from,
                  int recurse_further) {
  Ray *ray = this;

  v3 color = {};

  RayHit ray_hit = ray->get_object_hit(state, 0, INFINITY, reflected_from);

  if (ray_hit.object == NULL) {
    color = {0.05f, 0.05f, 0.05f};  // background color
    return color;
  }

  v3 hit_point = ray->point_at(ray_hit.at);
  v3 normal = ray_hit.object->get_normal(hit_point);
  v3 line_of_sight = -ray->direction.normalized();

  for (int i = 0; i < state->kLightCount; i++) {
    LightSource *light = &state->lights[i];

    v3 light_direction = (hit_point - light->source).normalized();

    b32 point_in_shadow = false;
    {
      // Cast shadow ray
      Ray shadow_ray = Ray();
      shadow_ray.origin = hit_point;
      shadow_ray.direction =
          light->source - hit_point;  // not normalizing on purpose

      RayHit shadow_ray_hit =
          shadow_ray.get_object_hit(state, 0, 1, ray_hit.object, true);
      if (shadow_ray_hit.object != NULL) {
        point_in_shadow = true;
      }
    }

    if (!point_in_shadow) {
      v3 V = (-light_direction + line_of_sight).normalized();

      r32 illuminance = -light_direction * normal;
      if (illuminance < 0) {
        illuminance = 0;
      }
      color += ray_hit.object->color * light->intensity * illuminance;

      // Calculate specular reflection
      r32 reflection = V * normal;
      if (reflection < 0) {
        reflection = 0;
      }
      v3 specular_reflection = ray_hit.object->specular_color *
                               light->intensity *
                               (r32)pow(reflection, ray_hit.object->phong_exp);
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
    r32 k = ray_hit.object->phong_exp * 0.001f;
    if (k > max_k) {
      k = max_k;
    }
    color +=
        k *
        reflection_ray.get_color(state, ray_hit.object, recurse_further - 1);
  }

  return color;
}
