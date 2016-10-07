#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

struct Pixel_Buffer {
  int width;
  int height;
  int max_width;
  int max_height;
  void *memory;
};

struct update_result {
  // empty for now
};

typedef b32 button_state;

struct user_input {
  union {
    button_state buttons[8];
    struct {
      button_state up;
      button_state down;
      button_state left;
      button_state right;
      button_state mouse_left;
      button_state mouse_middle;
      button_state mouse_right;

      button_state terminator;
    };
  };

  v2i mouse;
};

struct Area {
  // Multi-purpose editor area
  int left;
  int right;
  int top;
  int bottom;

  v3 color;
  b32 being_resized;

  Area() {};
  Area(v2i p1, v2i p2, v3 color);
  void draw(Pixel_Buffer *);
};

struct ProgramState {
  b32 initialized;

  Sphere *spheres;
  Plane *planes;
  Triangle *triangles;
  RayObject **ray_objects;

  LightSource *lights;

  RayCamera camera;

  // Some constants
  int kWindowWidth;
  int kWindowHeight;
  int kMaxRecursion;
  int kSphereCount;
  int kPlaneCount;
  int kTriangleCount;
  int kRayObjCount;
  int kLightCount;

  Area panel1;

  ProgramState() {
    // panel1 = {};
  };
};

update_result UpdateAndRender(Pixel_Buffer *pixel_buffer, user_input *Input);

#endif  // ED_CORE_H
