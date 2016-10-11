#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

#define MAX_INTERNAL_MEMORY_SIZE (1024 * 1024)

struct Pixel_Buffer {
  int width;
  int height;
  int max_width;
  int max_height;
  void *memory;
};

struct Update_Result {
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

struct Rect {
  int left;
  int right;
  int top;
  int bottom;
};

struct Area {
  // Multi-purpose editor area
  Rect rect;
  v3 color;

  Area() {};
  Area(v2i p1, v2i p2, v3 color);
  void draw(Pixel_Buffer *);
};

#define AREA_SPLITTER_MAX_AREAS 5

struct Area_Splitter {
  int position;
  bool being_moved;
  bool is_vertical;
  int one_side_count;
  int other_side_count;
  Area *one_side_areas[AREA_SPLITTER_MAX_AREAS];
  Area *other_side_areas[AREA_SPLITTER_MAX_AREAS];

  bool is_mouse_over(v2i mouse);
  void move(v2i mouse);
};

struct Program_State {
  Sphere *spheres;
  Plane *planes;
  Triangle *triangles;
  RayObject **ray_objects;

  LightSource *lights;

  RayCamera camera;

  Area area1;
  Area area2;

  Area_Splitter splitter1;

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
