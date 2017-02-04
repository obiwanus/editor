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

char *Editor_Names[Area_Editor_Type__COUNT] = {
  "3D view",
  "Ray trace",
};

struct Editor_3DView : Area_Editor {
  Camera camera;
  Editor_3DView_Mode mode;

  void update(Program_State *, User_Input *);
  void draw(Pixel_Buffer *, r32 *, Program_State *);
};

struct Editor_Raytrace : Area_Editor {
  Pixel_Buffer backbuffer;

  void update(User_Input *);
  void draw(Pixel_Buffer *, Program_State *);
  void trace_tile(Model *, v2i, v2i);
};

struct Raytrace_Work_Entry {
  Editor_Raytrace *editor;
  Model *models;
  v2i start;
  v2i end;
};

struct Raytrace_Work_Queue {
  u32 volatile next_entry_to_do;
  u32 volatile next_entry_to_add;

  Raytrace_Work_Entry entries[256];

  virtual void add_entry(Raytrace_Work_Entry) = 0;
};

#endif  // __ED_EDITORS_H__
