#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/stb_stretchy_buffer.h"
#include "ED_base.h"
#include "ED_core.h"
#include "ED_math.h"
#include "ED_model.h"
#include "raytrace/ED_raytrace.h"

void *Program_Memory::allocate(size_t size) {
  // Deallocation is not intended (yet)
  void *result;
  if (this->allocated + size > MAX_INTERNAL_MEMORY_SIZE) {
    return NULL;
  }
  result = this->free_memory;
  this->free_memory = (void *)((u8 *)this->free_memory + size);
  return result;
}

void Program_State::init(Program_Memory *memory) {
  Program_State *state = this;

  state->kWindowWidth = 1200;
  state->kWindowHeight = 900;

  memset(&state->UI, 0, sizeof(state->UI));
  state->UI.memory = memory;
  Area *area = state->UI.create_area(
      NULL, {0, 0, state->kWindowWidth, state->kWindowHeight});

  Model model;

  model.read_from_obj_file("../models/african_head/african_head.wobj");
  model.read_texture("../models/african_head/african_head_diffuse.jpg");
  sb_push(Model *, state->models, model);

  model.read_from_obj_file("../models/cube/cube.wobj");
  model.read_texture("../models/cube/cube.png");
  sb_push(Model *, state->models, model);

  // model.read_from_obj_file("../models/capsule/capsule.wobj");
  // model.read_texture("../models/capsule/capsule0.jpg");
  // sb_push(Model *, state->models, model);

  // Main ray tracer (tmp)
  Ray_Tracer *rt = &state->ray_tracer;
  rt->kMaxRecursion = 3;
  rt->kSphereCount = 3;
  rt->kPlaneCount = 1;
  rt->kTriangleCount = 0;
  rt->kRayObjCount = rt->kSphereCount + rt->kPlaneCount;
  rt->kLightCount = 3;

  // Spheres
  rt->spheres = new Sphere[rt->kSphereCount];

  rt->spheres[0].center = {350, 0, -1300};
  rt->spheres[0].radius = 300;
  rt->spheres[0].color = {0.7f, 0.7f, 0.7f};
  rt->spheres[0].phong_exp = 10;

  rt->spheres[1].center = {-400, 100, -1500};
  rt->spheres[1].radius = 400;
  rt->spheres[1].color = {0.2f, 0.2f, 0.2f};
  rt->spheres[1].phong_exp = 500;

  rt->spheres[2].center = {-500, -200, -1000};
  rt->spheres[2].radius = 100;
  rt->spheres[2].color = {0.05f, 0.05f, 0.05f};
  rt->spheres[2].phong_exp = 1000;

  // Planes
  rt->planes = new Plane[rt->kPlaneCount];

  rt->planes[0].point = {0, -300, 0};
  rt->planes[0].normal = {0, 1, 0};
  rt->planes[0].normal = rt->planes[0].normal.normalized();
  rt->planes[0].color = {0.2f, 0.3f, 0.4f};
  rt->planes[0].phong_exp = 1000;

  // Get a list of all objects
  rt->ray_objects =
      (RayObject **)malloc(rt->kRayObjCount * sizeof(RayObject *));
  {
    RayObject **ro_pointer = rt->ray_objects;
    for (int i = 0; i < rt->kSphereCount; i++) {
      *ro_pointer++ = &rt->spheres[i];
    }
    for (int i = 0; i < rt->kPlaneCount; i++) {
      *ro_pointer++ = &rt->planes[i];
    }
  }

  // Light
  rt->lights = new LightSource[rt->kLightCount];

  rt->lights[0].intensity = 0.7f;
  rt->lights[0].source = {1730, 600, -200};

  rt->lights[1].intensity = 0.4f;
  rt->lights[1].source = {-300, 1000, -100};

  rt->lights[2].intensity = 0.4f;
  rt->lights[2].source = {-1700, 300, 100};

  // Camera dimensions
  rt->camera.origin = {0, 0, 1000};
  rt->camera.left = -state->kWindowWidth / 2;
  rt->camera.right = state->kWindowWidth / 2;
  rt->camera.bottom = -state->kWindowHeight / 2;
  rt->camera.top = state->kWindowHeight / 2;
}

Update_Result update_and_render(Program_Memory *program_memory,
                                Program_State *state,
                                Pixel_Buffer *pixel_buffer, User_Input *input) {
  Update_Result result = {};

  // Remember where dragging starts
  for (int i = 0; i < 3; ++i) {
    if (input->button_went_down((Input_Button)i)) {
      input->mouse_positions[i] = input->mouse;
    }
  }

  result = state->UI.update_and_draw(pixel_buffer, input, state->models);

  return result;
}
