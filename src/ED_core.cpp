#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ED_base.h"
#include "ED_core.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

global void *_free_memory;  // for the allocator
global size_t _allocated;

global Program_State *g_state;

void *global_allocate(size_t size) {
  // Deallocation is not intended
  void *result;
  if (_allocated + size > MAX_INTERNAL_MEMORY_SIZE) {
    return NULL;
  }
  result = _free_memory;
  _free_memory = (void *)((u8 *)_free_memory + size);
  return result;
}

Update_Result update_and_render(void *program_memory,
                                Pixel_Buffer *pixel_buffer, User_Input *input) {
  Update_Result result = {};

  if (g_state == NULL) {
    _free_memory = program_memory;
    g_state = (Program_State *)global_allocate(sizeof(Program_State));

    g_state->kWindowWidth = 1000;
    g_state->kWindowHeight = 700;

    g_state->UI = {};
    g_state->UI.create_area(
        NULL, {0, 0, g_state->kWindowWidth, g_state->kWindowHeight});

#if 1
    g_state->kMaxRecursion = 3;
    g_state->kSphereCount = 3;
    g_state->kPlaneCount = 1;
    g_state->kTriangleCount = 0;
    g_state->kRayObjCount = g_state->kSphereCount + g_state->kPlaneCount;
    g_state->kLightCount = 3;

    // Spheres
    Sphere *spheres = new Sphere[g_state->kSphereCount];

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
    spheres[2].color = {0.2f, 0.2f, 0.3f};
    spheres[2].phong_exp = 1000;

    // Planes
    Plane *planes = new Plane[g_state->kPlaneCount];

    planes[0].point = {0, -300, 0};
    planes[0].normal = {0, 1, 0};
    planes[0].normal = planes[0].normal.normalized();
    planes[0].color = {0.4f, 0.3f, 0.2f};
    planes[0].phong_exp = 1000;

    // Get a list of all objects
    RayObject **ray_objects =
        (RayObject **)malloc(g_state->kRayObjCount * sizeof(RayObject *));
    {
      RayObject **ro_pointer = ray_objects;
      for (int i = 0; i < g_state->kSphereCount; i++) {
        *ro_pointer++ = &spheres[i];
      }
      for (int i = 0; i < g_state->kPlaneCount; i++) {
        *ro_pointer++ = &planes[i];
      }
    }

    // Light
    LightSource *lights = new LightSource[g_state->kLightCount];

    lights[0].intensity = 0.7f;
    lights[0].source = {1730, 600, -200};

    lights[1].intensity = 0.4f;
    lights[1].source = {-300, 1000, -100};

    lights[2].intensity = 0.4f;
    lights[2].source = {-1700, 300, 100};

    // Camera dimensions
    RayCamera *camera = &g_state->camera;
    camera->origin = {0, 0, 1000};
    camera->pixel_count = {g_state->kWindowWidth, g_state->kWindowHeight};
    camera->left = -camera->pixel_count.x / 2;
    camera->right = camera->pixel_count.x / 2;
    camera->bottom = -camera->pixel_count.y / 2;
    camera->top = camera->pixel_count.y / 2;

    g_state->spheres = spheres;
    g_state->planes = planes;

    g_state->ray_objects = ray_objects;

    g_state->lights = lights;
#endif
  }

  User_Interface *ui = &g_state->UI;
  ui->update_and_draw(pixel_buffer, input);

#if 1
  RayCamera *camera = &g_state->camera;
  LightSource *lights = g_state->lights;
  RayObject **ray_objects = g_state->ray_objects;

  for (int x = 0; x < pixel_buffer->width; x++) {
    for (int y = 0; y < pixel_buffer->height; y++) {
      Ray ray = camera->get_ray_through_pixel(x, pixel_buffer->height - y);

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      color += ray.get_color(g_state, 0, g_state->kMaxRecursion);

      // Crop
      for (int i = 0; i < 3; i++) {
        if (color.E[i] > 1) {
          color.E[i] = 1;
        }
        if (color.E[i] < 0) {
          color.E[i] = 0;
        }
      }

      draw_pixel(pixel_buffer, V2i(x, y), get_rgb_u32(color));
    }
  }

#endif  // if 0

  return result;
}
