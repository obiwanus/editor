#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

struct pixel_buffer {
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

  v3 mouse;
};

struct program_state {
  b32 initialized;

  Ray *ray;
  RayScreen *screen;

  Sphere *spheres;
  Plane *planes;
  Triangle *triangles;
  RayObject **ray_objects;

  LightSource *light;

  program_state() {

  }
};

update_result UpdateAndRender(pixel_buffer *PixelBuffer, user_input *Input);

#endif  // ED_CORE_H
