#ifndef __ED_UI_H__
#define __ED_UI_H__

struct Program_State;

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

enum Editor_3DView_Mode {
  Editor_3DView_Mode_Normal = 0,
  Editor_3DView_Mode_Camera_Rotate,
  Editor_3DView_Mode_Pivot_Move,
};

struct Editor_3DView : Area_Editor {
  Camera camera;
  Editor_3DView_Mode mode;

  void draw(Program_State *, User_Input *);
};

struct Editor_Raytrace : Area_Editor {
  void draw(User_Interface *);
};

struct Area {
  // Multi-purpose editor area
  int left;
  int top;
  int right;
  int bottom;

  Area *parent_area;
  Area_Splitter *splitter = NULL;
  Pixel_Buffer buffer;
  r32 *z_buffer = NULL;

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

  v3 cursor;

  Area *create_area(Area *, Rect);
  void remove_area(Area *);
  Area_Splitter *split_area(Area *, v2i, bool);
  void set_movement_boundaries(Area_Splitter *);
  void resize_window(int, int);
  Update_Result update_and_draw(Pixel_Buffer *, User_Input *, Program_State *);
};

#endif  // __ED_UI_H__
