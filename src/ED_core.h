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

  int prev_width;
  int prev_height;
  bool was_resized;

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
  int top;
  int right;
  int bottom;

  inline int get_width();
  inline int get_height();
  bool is_within(v2i point);
};

struct Area_Splitter;  // damned C++

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area_Splitter *splitter = NULL;

  inline int get_width();
  inline int get_height();

  inline Rect get_rect();
  inline void set_rect(Rect);

  void set_left(int);
  void set_right(int);
  void set_top(int);
  void set_bottom(int);

  void draw(Pixel_Buffer *);
};

struct Area_Splitter {
  int position;
  bool being_moved;
  bool is_vertical;

  Area *parent_area;
  Area *areas[2];  // it always splits an area in 2

  Rect get_rect();
  bool is_mouse_over(v2i mouse);
  void move(v2i mouse);
};

#define EDITOR_MAX_AREA_COUNT 30

struct User_Interface {
  bool splitter_move_in_progress;
  int num_areas;
  int num_splitters;
  Area areas[EDITOR_MAX_AREA_COUNT];
  Area_Splitter splitters[EDITOR_MAX_AREA_COUNT];

  Area *create_area(Rect, Area_Splitter *);
  Area_Splitter *_new_splitter(Area *);
  Area_Splitter *vertical_split(Area *, int);
  Area_Splitter *horizontal_split(Area *, int);
  void resize_window(int, int);
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
