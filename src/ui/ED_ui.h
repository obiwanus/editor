#ifndef __ED_UI_H__
#define __ED_UI_H__

struct Program_State;
struct Area_Splitter;
struct Area;

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

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area *parent_area;
  Area_Splitter *splitter = NULL;

  Pixel_Buffer *buffer;  // just a pointer to the global buffer

  Area_Editor_Type editor_type;
  Editor_3DView editor_3dview;
  Editor_Raytrace editor_raytrace;

  UI_Select type_select;

  static const int kPanelHeight = 24;

  inline int get_width();
  inline int get_height();

  inline Rect get_client_rect();

  inline Rect get_rect();
  inline void set_rect(Rect);

  Rect get_split_handle(int);
  Rect get_delete_button();
  bool mouse_over_split_handle(v2i);
  bool mouse_over_delete_button(v2i);

  bool is_visible();

  void set_left(int);
  void set_right(int);
  void set_top(int);
  void set_bottom(int);
  void reposition_splitter(r32, r32);

  void destroy();
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

  r32 *z_buffer;
  Pixel_Buffer *buffer;

  v3 cursor;

  Area *create_area(Area *, Rect, bool);
  void remove_area(Area *);
  Area_Splitter *split_area(Area *, v2i, bool);
  void set_movement_boundaries(Area_Splitter *);
  void resize_window(int, int);
  Update_Result update_and_draw(User_Input *, Program_State *);
};

#endif  // __ED_UI_H__
