#ifndef __ED_EDITORS_H__
#define __ED_EDITORS_H__

struct Program_State;
struct Area;

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
  void draw();
};

#endif  // __ED_EDITORS_H__
