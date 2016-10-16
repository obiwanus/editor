#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_math.h"
#include "ED_ui.h"
#include "raytrace/ED_raytrace.h"

#define MAX_INTERNAL_MEMORY_SIZE (1024 * 1024)


struct Update_Result {
  // empty for now
};

struct Program_State {
  Sphere *spheres;
  Plane *planes;
  Triangle *triangles;
  RayObject **ray_objects;

  LightSource *lights;

  RayCamera camera;

  User_Interface UI;

  // Some constants
  int kWindowWidth;
  int kWindowHeight;
  int kMaxRecursion;
  int kSphereCount;
  int kPlaneCount;
  int kTriangleCount;
  int kRayObjCount;
  int kLightCount;
};

Update_Result update_and_render(void *, Pixel_Buffer *, user_input *);

#endif  // ED_CORE_H
