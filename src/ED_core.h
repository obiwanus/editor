#ifndef ED_CORE_H
#define ED_CORE_H

// 256 Mb
#define MAX_INTERNAL_MEMORY_SIZE (256 * 1024 * 1024)

struct Program_Memory {
  void *start;
  void *free_memory;
  size_t allocated;

  // TODO: do something with allocations finally
  void *allocate(size_t);
};

template <typename T>
void swap(T &p1, T &p2) {
  T buf = p1;
  p1 = p2;
  p2 = buf;
}

struct User_Interface;
struct Model;

enum Input_Button {
  IB_mouse_left = 0,
  IB_mouse_middle,
  IB_mouse_right,

  IB_up,
  IB_down,
  IB_left,
  IB_right,

  IB_shift,
  IB_key,

  IB__COUNT,
};

struct User_Input {
  bool buttons[IB__COUNT];
  bool key_pressed;
  int symbol;

  // Store the last position mouse was in when a button went down
  v2i mouse_positions[3];
  v2i mouse;

  int scroll = 0;

  User_Input *old;

  bool button_is_down(Input_Button);
  bool button_was_down(Input_Button);
  bool button_went_down(Input_Button);
  bool button_went_up(Input_Button);
  bool key_went_down(int);
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

struct Program_State {
  int kWindowWidth;
  int kWindowHeight;

  User_Interface *UI;

  Model *models = NULL;
  Model *selected_model = NULL;

  void init(Program_Memory *);
};

struct ED_Font_Codepoint {
  int width;
  int height;
  u8 *bitmap;
};

struct ED_Font {
  stbtt_fontinfo info;
  static const char first_char = '!';
  static const char last_char = '~';
  ED_Font_Codepoint codepoints[last_char - first_char + 1];
  int baseline;
  int line_height;
  u8 *bitmap;

  void load_from_file(char *, int);
};

Update_Result update_and_render(Program_Memory *, Program_State *,
                                Pixel_Buffer *, User_Input *);

// ============================== Globals =====================================

global bool g_running;
global Pixel_Buffer g_pixel_buffer;
global Program_Memory g_program_memory;
global ED_Font g_font;

#endif  // ED_CORE_H
