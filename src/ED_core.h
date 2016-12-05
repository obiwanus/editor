#ifndef ED_CORE_H
#define ED_CORE_H

#include "ED_base.h"
#include "ED_ui.h"
#include "ED_model.h"
#include "raytrace/ED_raytrace.h"

struct Program_State {
  // Tmp mesh
  Mesh mesh;

  int kWindowWidth;
  int kWindowHeight;

  User_Interface UI;

  Model model;

  Ray_Tracer ray_tracer;

  void init(Program_Memory *);
};

Update_Result update_and_render(Program_Memory *, Program_State *,
                                Pixel_Buffer *, User_Input *);

#endif  // ED_CORE_H
