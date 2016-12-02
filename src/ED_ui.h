#ifndef __ED_UI_H__
#define __ED_UI_H__

#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"


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

struct Rect {
  int left;
  int top;
  int right;
  int bottom;

  inline int get_width();
  inline int get_height();
  bool contains(v2i point);
  v2i projected(v2i point);
};

struct Pixel_Buffer {
  int width;
  int height;
  int max_width;
  int max_height;

  bool was_resized;

  void *memory;

  void allocate(Program_Memory *);
  Rect get_rect();
};

struct Area_Splitter;  // damned C++
struct Area;           // bloody C++

struct UI_Select {
  // Simpler than flags
  bool align_right;
  bool align_bottom;
  bool highlighted;
  bool open;

  int option_count;
  int option_height;

  int option_selected;

  int x;
  int y;
  Area *parent_area;

  Rect get_rect();
};

enum Area_Editor_Type {
  Area_Editor_Type_Empty = 0,
  Area_Editor_Type_Raytrace,

  Area_Editor_Type__COUNT,
};

struct Area_Editor {
  Area *area;

  void update_and_draw(Pixel_Buffer *, User_Input *){};
};

struct Editor_Empty : Area_Editor {
  void draw();
};

struct Editor_Raytrace : Area_Editor {
  void draw(Ray_Tracer *);
};

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area *parent_area;
  Area_Splitter *splitter = NULL;
  Pixel_Buffer *draw_buffer = NULL;

  Area_Editor_Type editor_type;
  Editor_Empty editor_empty;
  Editor_Raytrace editor_raytrace;

#define AREA_PANEL_HEIGHT 26

  inline int get_width();
  inline int get_height();

  inline Rect get_client_rect();

  inline Rect get_rect();
  inline void set_rect(Rect);

  Rect get_split_handle(int);
  Rect get_delete_button();
  bool mouse_over_split_handle(v2i);

  bool is_visible();

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

enum Cursor_Type {
  Cursor_Type_Arrow = 0,
  Cursor_Type_Cross,
  Cursor_Type_Hand,
  Cursor_Type_Resize_Vert,
  Cursor_Type_Resize_Horiz,

  Cursor_Type__COUNT,
};

struct Update_Result {
  Cursor_Type cursor;
};

struct User_Interface {
  Program_Memory *memory;

  v2i pointer_start;

  // Areas and splitters
  int num_splitters;
  int num_selects;
  int num_areas;

  Area **areas;
  Area_Splitter **splitters;
  UI_Select **selects;

  bool can_pick_splitter;
  bool can_split_area;
  bool can_pick_select;
  bool can_delete_area;
  Area *area_being_split;
  Area_Splitter *splitter_being_moved;

  Area *create_area(Area *, Rect, Pixel_Buffer *buf = NULL);
  void remove_area(Area *);
  Area_Splitter *_new_splitter(Area *);
  Area_Splitter *vertical_split(Area *, int);
  Area_Splitter *horizontal_split(Area *, int);
  UI_Select *new_type_selector(Area *);
  void _split_type_selectors(Area *, Area_Splitter *, bool);
  void set_movement_boundaries(Area_Splitter *);
  void resize_window(int, int);
  Update_Result update_and_draw(Pixel_Buffer *, User_Input *);
  void draw_areas(Ray_Tracer *);
};


inline u32 get_rgb_u32(v3);
inline void draw_pixel(Pixel_Buffer *, v2i, u32);

#endif  // __ED_UI_H__
