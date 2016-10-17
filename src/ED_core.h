#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_ui.h"
#include "raytrace/ED_raytrace.h"

// 512 Mb
#define MAX_INTERNAL_MEMORY_SIZE (512 * 1024 * 1024)


struct Update_Result {
  // empty for now
};

struct Program_State {

  // Tmp raytracing
  Sphere *spheres;
  Plane *planes;
  Triangle *triangles;
  RayObject **ray_objects;
  LightSource *lights;
  RayCamera camera;

  // Tmp mesh
  Mesh mesh;

  User_Interface UI;

  // Some constants - tmp too
  int kWindowWidth;
  int kWindowHeight;
  int kMaxRecursion;
  int kSphereCount;
  int kPlaneCount;
  int kTriangleCount;
  int kRayObjCount;
  int kLightCount;
};

Update_Result update_and_render(void *, Pixel_Buffer *, User_Input *);

#endif  // ED_CORE_H
