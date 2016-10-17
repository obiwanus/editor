#ifndef __ED_UI_H__
#define __ED_UI_H__

#include "ED_base.h"
#include "ED_math.h"

// -------- TODO: move out somewhere -----------------

struct Vertex {
  v3 position;
};

struct Triangle {
  v3 color;
  int vertices[3];
};

// struct Edge {

// };

struct Mesh {
  void *memory_block = NULL;

  int num_vertices;
  Vertex *vertices;

  int num_triangles;
  Triangle *triangles;
};

// -------- /TODO: move out somewhere ----------------

typedef b32 button_state;

struct User_Input {
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

struct Rect {
  int left;
  int top;
  int right;
  int bottom;

  inline int get_width();
  inline int get_height();
  bool contains(v2i point);
};

struct Area_Splitter;  // damned C++
struct Area;  // bloody C++

enum Area_Editor_Type {
  Area_Editor_Type_Empty = 0,
  Area_Editor_Type_Raytrace,
};

struct Area_Editor {
  Area *area;

  void update(User_Input *) {};
  void draw(Pixel_Buffer *) {};
};

struct Editor_Empty : Area_Editor {
  void draw(Pixel_Buffer *);
};

struct Editor_Raytrace : Area_Editor {};

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area *parent_area;
  Area_Splitter *splitter = NULL;

  Area_Editor_Type editor_type;
  Editor_Empty editor_empty;
  Editor_Raytrace editor_raytrace;

  inline int get_width();
  inline int get_height();

  inline Rect get_rect();
  inline void set_rect(Rect);

  Rect get_split_handle(int);
  bool mouse_over_split_handle(v2i);

  void set_left(int);
  void set_right(int);
  void set_top(int);
  void set_bottom(int);

  void draw(Pixel_Buffer *);
};

struct Area_Splitter {
  int position;
  int position_min;  // to restrict movement
  int position_max;

  bool is_vertical;

  Area *parent_area;
  Area *areas[2];  // it always splits an area in 2

  Rect get_rect();
  bool is_mouse_over(v2i);
  bool is_under(Area *);
  void move(v2i mouse);
};

#define EDITOR_MAX_AREA_COUNT 50

struct User_Interface {
  int num_areas;
  int num_splitters;

  v2i pointer_start;
  bool can_pick_splitter;
  bool can_split_area;
  Area *area_being_split;
  Area_Splitter *splitter_being_moved;

  Area areas[EDITOR_MAX_AREA_COUNT];
  Area_Splitter splitters[EDITOR_MAX_AREA_COUNT];

  Area *create_area(Area *, Rect);
  Area_Splitter *_new_splitter(Area *);
  Area_Splitter *vertical_split(Area *, int);
  Area_Splitter *horizontal_split(Area *, int);
  void set_movement_boundaries(Area_Splitter *);
  void resize_window(int, int);
  void update_and_draw(Pixel_Buffer *, User_Input *);
};

inline u32 get_rgb_u32(v3);
inline void draw_pixel(Pixel_Buffer *, v2i, u32);

#endif  // __ED_UI_H__
