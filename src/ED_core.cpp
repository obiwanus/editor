#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ED_base.h"
#include "ED_core.h"
#include "ED_math.h"
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

Update_Result update_and_render(Program_Memory *program_memory, Program_State *state,
                                Pixel_Buffer *pixel_buffer, User_Input *input) {
  Update_Result result = {};

  if (!state->is_initialized) {
    state->is_initialized = true;

    state->kWindowWidth = 1000;
    state->kWindowHeight = 700;

    state->UI = {};
    Area *area = state->UI.create_area(
        NULL, {0, 0, state->kWindowWidth, state->kWindowHeight});

    Area_Splitter *splitter =
        state->UI.vertical_split(area, area->get_width() / 2);
    splitter->areas[0]->editor_type = Area_Editor_Type_Raytrace;
    splitter->areas[1]->editor_type = Area_Editor_Type_Raytrace;

    state->kMaxRecursion = 3;
    state->kSphereCount = 3;
    state->kPlaneCount = 1;
    state->kTriangleCount = 0;
    state->kRayObjCount = state->kSphereCount + state->kPlaneCount;
    state->kLightCount = 3;

    // Spheres
    Sphere *spheres = new Sphere[state->kSphereCount];

    spheres[0].center = {350, 0, -1300};
    spheres[0].radius = 300;
    spheres[0].color = {0.7f, 0.7f, 0.7f};
    spheres[0].phong_exp = 10;

    spheres[1].center = {-400, 100, -1500};
    spheres[1].radius = 400;
    spheres[1].color = {0.2f, 0.2f, 0.2f};
    spheres[1].phong_exp = 500;

    spheres[2].center = {-500, -200, -1000};
    spheres[2].radius = 100;
    spheres[2].color = {0.05f, 0.05f, 0.05f};
    spheres[2].phong_exp = 1000;

    // Planes
    Plane *planes = new Plane[state->kPlaneCount];

    planes[0].point = {0, -300, 0};
    planes[0].normal = {0, 1, 0};
    planes[0].normal = planes[0].normal.normalized();
    planes[0].color = {0.2f, 0.3f, 0.4f};
    planes[0].phong_exp = 1000;

    // Get a list of all objects
    RayObject **ray_objects =
        (RayObject **)malloc(state->kRayObjCount * sizeof(RayObject *));
    {
      RayObject **ro_pointer = ray_objects;
      for (int i = 0; i < state->kSphereCount; i++) {
        *ro_pointer++ = &spheres[i];
      }
      for (int i = 0; i < state->kPlaneCount; i++) {
        *ro_pointer++ = &planes[i];
      }
    }

    // Light
    LightSource *lights = new LightSource[state->kLightCount];

    lights[0].intensity = 0.7f;
    lights[0].source = {1730, 600, -200};

    lights[1].intensity = 0.4f;
    lights[1].source = {-300, 1000, -100};

    lights[2].intensity = 0.4f;
    lights[2].source = {-1700, 300, 100};

    // Camera dimensions
    RayCamera *camera = &state->camera;
    camera->origin = {0, 0, 1000};
    camera->left = -state->kWindowWidth / 2;
    camera->right = state->kWindowWidth / 2;
    camera->bottom = -state->kWindowHeight / 2;
    camera->top = state->kWindowHeight / 2;

    state->spheres = spheres;
    state->planes = planes;

    state->ray_objects = ray_objects;

    state->lights = lights;
  }

  User_Interface *ui = &state->UI;
  result = ui->update_and_draw(pixel_buffer, input, state);

  return result;
}
