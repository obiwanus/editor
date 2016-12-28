#ifndef __ED_UI_H__
#define __ED_UI_H__

#include "ED_base.h"
#include "ED_math.h"
#include "ED_model.h"
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

enum Input_Button {
  IB_mouse_left = 0,
  IB_mouse_middle,
  IB_mouse_right,

  IB_up,
  IB_down,
  IB_left,
  IB_right,

  IB_toggle_projection,

  IB__COUNT,
};

struct User_Input {
  bool buttons[IB__COUNT];

  // Store the last position mouse was in when a button went down
  v2i mouse_positions[3];
  v2i mouse;

  int scroll = 0;

  User_Input *old;

  bool button_is_down(Input_Button);
  bool button_went_down(Input_Button);
  bool button_went_up(Input_Button);
};

struct Rect {
  int left;
  int top;
  int right;
  int bottom;

  inline int get_width();
  inline int get_height();
  bool contains(v2i point);
  v2i projected(v2i point, bool ui);
  v2i projected_to_area(v2i point);
  int get_area();
};

struct Pixel_Buffer {
  int width;
  int height;
  int max_width;
  int max_height;

  bool was_resized;

  void *memory;

  void allocate();
  Rect get_rect();
};

struct Area_Splitter;  // damned C++
struct Area;           // bloody C++
struct User_Interface;

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
  void update_and_draw(User_Input *);
};

enum Area_Editor_Type {
  Area_Editor_Type_3DView = 0,
  Area_Editor_Type_Raytrace,

  Area_Editor_Type__COUNT,
};

struct Area_Editor {
  Area *area;
  bool is_drawn;

  void update_and_draw(Pixel_Buffer *, User_Input *){};
};

struct Editor_3DView : Area_Editor {
  Camera camera;

  void draw(User_Interface *, Model, User_Input *);
};

struct Editor_Raytrace : Area_Editor {
  void draw(User_Interface *, Ray_Tracer *, Model);
};

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area *parent_area;
  Area_Splitter *splitter = NULL;
  Pixel_Buffer *buffer = NULL;

  Area_Editor_Type editor_type;
  Editor_3DView editor_3dview;
  Editor_Raytrace editor_raytrace;

  UI_Select type_select;

  static const int kPanelHeight = 26;

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

  void deallocate();
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

  // Areas and splitters
  int num_splitters;
  int num_areas;

  Area **areas;
  Area_Splitter **splitters;

  Area *active_area;  // where user did something last
  Area *area_being_split;
  Area *area_being_deleted;
  Area_Splitter *splitter_being_moved;

  Area *create_area(Area *, Rect);
  void remove_area(Area *);
  Area_Splitter *split_area(Area *, v2i, bool);
  void set_movement_boundaries(Area_Splitter *);
  void resize_window(int, int);
  Update_Result update_and_draw(Pixel_Buffer *, User_Input *, Model);
  void draw_areas(Ray_Tracer *, Model, User_Input *);
};

#endif  // __ED_UI_H__
