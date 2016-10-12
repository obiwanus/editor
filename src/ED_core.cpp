#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ED_core.h"
#include "ED_base.h"
#include "ED_math.h"
#include "raytrace/ED_raytrace.h"

global void *_free_memory;  // for the allocator
global size_t _allocated;

global Program_State *g_state;

void *allocate(size_t size) {
  // Deallocation is not intended
  void *result;
  if (_allocated + size > MAX_INTERNAL_MEMORY_SIZE) {
    return NULL;
  }
  result = _free_memory;
  _free_memory = (void *)((u8 *)_free_memory + size);
  return result;
}

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2i Point, u32 Color) {
  int x = Point.x;
  int y = Point.y;

  if (x < 0 || x > pixel_buffer->width || y < 0 || y > pixel_buffer->height) {
    return;
  }
  // y = pixel_buffer->height - y;  // Origin in bottom-left
  u32 *pixel = (u32 *)pixel_buffer->memory + x + y * pixel_buffer->width;
  *pixel = Color;
}

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2 Point, u32 Color) {
  // A v2 version
  v2i point = {(int)Point.x, (int)Point.y};
  draw_pixel(pixel_buffer, point, Color);
}

// TODO: draw triangles
void draw_line(Pixel_Buffer *pixel_buffer, v2i A, v2i B, u32 Color) {
  bool swapped = false;
  if (abs(B.x - A.x) < abs(B.y - A.y)) {
    int tmp = A.x;
    A.x = A.y;
    A.y = tmp;
    tmp = B.x;
    B.x = B.y;
    B.y = tmp;
    swapped = true;
  }
  if (B.x - A.x < 0) {
    v2i tmp = B;
    B = A;
    A = tmp;
  }

  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = dy >= 0 ? 1 : -1;
  int error = sign * dy - dx;
  int y = A.y;
  for (int x = A.x; x <= B.x; x++) {
    if (!swapped) {
      draw_pixel(pixel_buffer, V2i(x, y), Color);
    } else {
      draw_pixel(pixel_buffer, V2i(y, x), Color);
    }
    error += sign * dy;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_line(Pixel_Buffer *pixel_buffer, v2 A, v2 B, u32 Color) {
  v2i a = {(int)A.x, (int)A.y};
  v2i b = {(int)B.x, (int)B.y};
  draw_line(pixel_buffer, a, b, Color);
}

inline u32 get_rgb_u32(v3 Color) {
  assert(Color.r >= 0 && Color.r <= 1);
  assert(Color.g >= 0 && Color.g <= 1);
  assert(Color.b >= 0 && Color.b <= 1);

  u32 result = 0x00000000;
  u8 R = (u8)(Color.r * 255);
  u8 G = (u8)(Color.g * 255);
  u8 B = (u8)(Color.b * 255);
  result = R << 16 | G << 8 | B;
  return result;
}

void draw_rect(Pixel_Buffer *pixel_buffer, Rect rect, v3 color) {
  u32 rgb = get_rgb_u32(color);

  if (rect.left < 0) rect.left = 0;
  if (rect.top < 0) rect.top = 0;
  if (rect.right > pixel_buffer->width) rect.right = pixel_buffer->width;
  if (rect.bottom > pixel_buffer->height) rect.bottom = pixel_buffer->height;

  for (int x = rect.left; x < rect.right; x++) {
    for (int y = rect.top; y < rect.bottom; y++) {
      // Don't care about performance
      draw_pixel(pixel_buffer, V2i(x, y), rgb);
    }
  }
}

bool Rect::is_within(v2i point) {
  bool result = (this->left <= point.x) && (point.x <= this->right) &&
                (this->top <= point.y) && (point.y <= this->bottom);
  return result;
}

void Area::draw(Pixel_Buffer *pixel_buffer) {
  if (this->splitter) {
    // Don't draw the parent area
    this->splitter->areas[0]->draw(pixel_buffer);
    this->splitter->areas[1]->draw(pixel_buffer);
    return;
  }

  // draw outline
  int left = this->rect.left;
  int top = this->rect.top;
  int right = this->rect.right;
  int bottom = this->rect.bottom;

  draw_line(pixel_buffer, V2i(left, top), V2i(right, top), 0x00FFFFFF);
  draw_line(pixel_buffer, V2i(left, top), V2i(left, bottom), 0x00FFFFFF);
  draw_line(pixel_buffer, V2i(right, top), V2i(right, bottom), 0x00FFFFFF);
  draw_line(pixel_buffer, V2i(left, bottom), V2i(right, bottom), 0x00FFFFFF);
}

void Area::resize(int width, int height) {
  this->rect.right = this->rect.left + width;
  this->rect.bottom = this->rect.top + height;

  if (this->splitter != NULL) {
    // assert(!"resize splitter")
  }
}

Rect Area_Splitter::get_rect() {
  Rect result = {};
  // const int kSensitivity = 5;
  // assert(this->one_side_count > 0);
  // Area *area_for_reference = this->one_side_areas[0];

  // // Not sure if it's better than storing the rect
  // if (this->is_vertical) {
  //   result.left = this->position - kSensitivity;
  //   result.right = this->position + kSensitivity;
  //   result.top = area_for_reference->rect.top;
  //   result.bottom = area_for_reference->rect.bottom;
  // } else {
  //   result.top = this->position - kSensitivity;
  //   result.bottom = this->position + kSensitivity;
  //   result.left = area_for_reference->rect.left;
  //   result.right = area_for_reference->rect.right;
  // }

  return result;
}

bool Area_Splitter::is_mouse_over(v2i mouse) {
  bool result = this->get_rect().is_within(mouse);

  return result;
}

void Area_Splitter::move(v2i mouse) {
  // if (this->is_vertical) {
  //   this->position = mouse.x;
  //   for (int i = 0; i < this->one_side_count; i++) {
  //     this->one_side_areas[i]->rect.right = this->position;
  //   }
  //   for (int i = 0; i < this->other_side_count; i++) {
  //     this->other_side_areas[i]->rect.left = this->position;
  //   }
  // } else {
  //   this->position = mouse.y;
  //   for (int i = 0; i < this->one_side_count; i++) {
  //     this->one_side_areas[i]->rect.bottom = this->position;
  //   }
  //   for (int i = 0; i < this->other_side_count; i++) {
  //     this->other_side_areas[i]->rect.top = this->position;
  //   }
  // }
}

Area *User_Interface::create_area(Rect rect, Area_Splitter *splitter = NULL) {
  Area *area = &this->areas[this->num_areas];
  this->num_areas++;

  *area = {};
  area->rect = rect;
  area->splitter = splitter;

  return area;
}

Area_Splitter *User_Interface::_new_splitter(Area *area, int position) {
  Area_Splitter *splitter = &this->splitters[this->num_splitters];
  area->splitter = splitter;
  this->num_splitters++;

  *splitter = {};
  splitter->is_vertical = true;
  splitter->position = position;
  splitter->parent_area = area;

  return splitter;
}

Area_Splitter *User_Interface::vertical_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area, position);
  splitter->is_vertical = true;

  // Create 2 areas
  Rect rect = area->rect;
  rect.right = position;
  splitter->areas[0] = this->create_area(rect);

  rect = area->rect;
  rect.left = position;
  splitter->areas[1] = this->create_area(rect);

  return splitter;
}

Area_Splitter *User_Interface::horizontal_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area, position);
  splitter->is_vertical = false;

  // Create 2 areas
  Rect rect = area->rect;
  rect.bottom = position;
  splitter->areas[0] = this->create_area(rect);

  rect = area->rect;
  rect.top = position;
  splitter->areas[1] = this->create_area(rect);

  return splitter;
}

Update_Result update_and_render(void *program_memory,
                                Pixel_Buffer *pixel_buffer, user_input *input) {
  Update_Result result = {};

  if (g_state == NULL) {
    _free_memory = program_memory;
    g_state = (Program_State *)allocate(sizeof(Program_State));

    g_state->kWindowWidth = 1000;
    g_state->kWindowHeight = 700;
    // g_state->kMaxRecursion = 3;
    // g_state->kSphereCount = 3;
    // g_state->kPlaneCount = 1;
    // g_state->kTriangleCount = 0;
    // g_state->kRayObjCount =
    //     g_state->kSphereCount + g_state->kPlaneCount +
    //     g_state->kTriangleCount;
    // g_state->kLightCount = 3;

    User_Interface *ui = &g_state->UI;
    *ui = {};

    // Parent area
    Area *parent_area =
        ui->create_area({0, 0, g_state->kWindowWidth, g_state->kWindowHeight});
    Area_Splitter *splitter1 =
        ui->vertical_split(parent_area, g_state->kWindowWidth / 2);

    Area *left_area = splitter1->areas[0];
    Area_Splitter *splitter2 = ui->horizontal_split(left_area, left_area->rect.bottom / 3);

    // TMP
    // g_state->area1 =
    //     Area({0, 0}, {500, g_state->kWindowHeight}, V3(0.1f, 0.2f, 0.3f));
    // g_state->area2 = Area({500, 0}, {1000, 300}, V3(0.05f, 0.15f, 0.2f));
    // g_state->area3 = Area({500, 300}, {1000, g_state->kWindowHeight},
    //                       V3(0.15f, 0.25f, 0.35f));

    // g_state->splitter[0] = {};
    // g_state->splitter[0].is_vertical = true;
    // g_state->splitter[0].position = g_state->area1.rect.right;
    // g_state->splitter[0].one_side_count = 1;
    // g_state->splitter[0].one_side_areas[0] = &g_state->area1;
    // g_state->splitter[0].other_side_count = 2;
    // g_state->splitter[0].other_side_areas[0] = &g_state->area2;
    // g_state->splitter[0].other_side_areas[1] = &g_state->area3;

    // g_state->splitter[1] = {};
    // g_state->splitter[1].is_vertical = false;
    // g_state->splitter[1].position = g_state->area2.rect.bottom;
    // g_state->splitter[1].one_side_count = 1;
    // g_state->splitter[1].one_side_areas[0] = &g_state->area2;
    // g_state->splitter[1].other_side_count = 1;
    // g_state->splitter[1].other_side_areas[0] = &g_state->area3;
  }

  User_Interface *ui = &g_state->UI;

  assert(ui->num_areas > 0 && ui->num_areas < EDITOR_MAX_AREA_COUNT);
  assert(ui->num_splitters >= 0 && ui->num_splitters < EDITOR_MAX_AREA_COUNT);

  if (pixel_buffer->was_resized) {
    ui->areas[0].resize(pixel_buffer->width, pixel_buffer->height);
    pixel_buffer->was_resized = false;
  }

  // // Drag splitters with a mouse
  // for (int i = 0; i < g_state->kNumSplitters; i++) {
  //   Area_Splitter *splitter = &g_state->splitter[i];
  //   if (input->mouse_left && !splitter->being_moved &&
  //       splitter->is_mouse_over(input->mouse)) {
  //     splitter->being_moved = true;
  //   }
  //   if (!input->mouse_left) {
  //     splitter->being_moved = false;
  //   }
  //   if (splitter->being_moved) {
  //     splitter->move(input->mouse);
  //   }
  // }

  // Clear
  memset(pixel_buffer->memory, 0,
         pixel_buffer->width * pixel_buffer->height * 4);

  ui->areas[0].draw(pixel_buffer);  // draw the parent area

#if 0

  RayCamera *camera = &g_state->camera;

  if (!g_state->initialized) {
    g_state->initialized = true;

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

    // Triangles
    // Triangle *triangles = new Triangle[kTriangleCount];

    // triangles[0].a = {480, -250, -100};
    // triangles[0].b = {0, 150, -700};
    // triangles[0].c = {-300, -50, -10};
    // triangles[0].color = {0.3f, 0.3f, 0.1f};
    // triangles[0].phong_exp = 10;

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
      // for (int i = 0; i < kTriangleCount; i++) {
      //   *ro_pointer++ = &triangles[i];
      // }
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
    camera->origin = {0, 0, 1000};
    camera->pixel_count = {g_state->kWindowWidth, g_state->kWindowHeight};
    camera->left = -camera->pixel_count.x / 2;
    camera->right = camera->pixel_count.x / 2;
    camera->bottom = -camera->pixel_count.y / 2;
    camera->top = camera->pixel_count.y / 2;

    g_state->spheres = spheres;
    g_state->planes = planes;
    // g_state->triangles = triangles;

    g_state->ray_objects = ray_objects;

    g_state->lights = lights;
  }

  LightSource *lights = g_state->lights;
  RayObject **ray_objects = g_state->ray_objects;

  if (input->up) {
    g_state->lights[0].source.v += 100;
  }
  if (input->down) {
    g_state->lights[0].source.v -= 100;
  }
  if (input->left) {
    g_state->lights[0].source.u -= 100;
  }
  if (input->right) {
    g_state->lights[0].source.u += 100;
  }

  if (input->mouse_left) {
    Ray pointer_ray =
        camera->get_ray_through_pixel((int)input->mouse.x, (int)input->mouse.y);

    RayHit pointer_hit =
        pointer_ray.get_object_hit(&g_state, 0, INFINITY, NULL);
    if (pointer_hit.object != NULL) {
      pointer_hit.object->color.x += 0.1f;
    }
  }

  for (int x = 0; x < pixel_buffer->width; x++) {
    for (int y = 0; y < pixel_buffer->height; y++) {
      Ray ray = camera->get_ray_through_pixel(x, y);

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      color += ray.get_color(&g_state, 0, g_state->kMaxRecursion);

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
