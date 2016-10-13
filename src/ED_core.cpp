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

inline int Rect::get_width() {
  int result = this->right - this->left;
  assert(result >= 0);
  return result;
}

inline int Rect::get_height() {
  int result = this->bottom - this->top;
  assert(result >= 0);
  return result;
}

void Area::draw(Pixel_Buffer *pixel_buffer) {
  if (this->splitter) {
    // Only draw the child areas
    this->splitter->areas[0]->draw(pixel_buffer);
    this->splitter->areas[1]->draw(pixel_buffer);
    return;
  }

  // draw outline
  int right = this->left + this->width - 1;
  int bottom = this->top + this->height - 1;

  draw_line(pixel_buffer, V2i(this->left, this->top), V2i(right, this->top),
            0x00FFFFFF);
  draw_line(pixel_buffer, V2i(this->left, this->top), V2i(this->left, bottom),
            0x00FFFFFF);
  draw_line(pixel_buffer, V2i(right, this->top), V2i(right, bottom),
            0x00FFFFFF);
  draw_line(pixel_buffer, V2i(this->left, bottom), V2i(right, bottom),
            0x00FFFFFF);
}

Rect Area::get_rect() {
  Rect rect;

  rect.left = this->left;
  rect.top = this->top;
  rect.right = this->left + this->width;
  rect.bottom = this->top + this->height;

  return rect;
}

void Area::set_rect(Rect rect) {
  this->left = rect.left;
  this->top = rect.top;
  this->width = rect.right - rect.left;
  this->height = rect.bottom - rect.top;
}

void Area::resize_width(int new_width) {
  // Resizes area and all child areas proportionally
  if (new_width <= 0) return;
  int old_width = this->width;
  this->width = new_width;
  if (this->splitter != NULL) {
    Area *area1 = this->splitter->areas[0];
    Area *area2 = this->splitter->areas[1];
    if (this->splitter->is_vertical) {
      int s_rel_pos = this->splitter->position - this->left;
      s_rel_pos = (int)((r32)new_width * ((r32)s_rel_pos / (r32)old_width));
      this->splitter->position = this->left + s_rel_pos;
      area1->left = this->left;
      area1->resize_width(s_rel_pos);
      area2->left = this->splitter->position;
      area2->resize_width(new_width - s_rel_pos);
    } else {
      area1->left = area2->left = this->left;
      area1->resize_width(new_width);
      area2->resize_width(new_width);
    }
  }
}

void Area::resize_height(int new_height) {
  // Resizes area and all child areas proportionally
  if (new_height <= 0) return;
  int old_height = this->height;
  this->height = new_height;
  if (this->splitter != NULL) {
    Area *area1 = this->splitter->areas[0];
    Area *area2 = this->splitter->areas[1];
    if (!this->splitter->is_vertical) {
      int s_rel_pos = this->splitter->position - this->top;
      s_rel_pos = (int)((r32)new_height * ((r32)s_rel_pos / (r32)old_height));
      this->splitter->position = this->top + s_rel_pos;
      area1->top = this->top;
      area1->resize_height(s_rel_pos);
      area2->top = this->splitter->position;
      area2->resize_height(new_height - s_rel_pos);
    } else {
      area1->top = area2->top = this->top;
      area1->resize_height(new_height);
      area2->resize_height(new_height);
    }
  }
}

void Area::resize(int new_width, int new_height) {
  if (new_width != this->width) {
    this->resize_width(new_width);
  }
  if (new_height != this->height) {
    this->resize_height(new_height);
  }
}

void Area::set_left(int position) {
  int right = this->left + this->width;
  this->left = position;
  this->width = right - position;  // preserve right
  if (this->splitter == NULL) return;
  this->splitter->areas[0]->set_left(position);
  if (!this->splitter->is_vertical) {
    this->splitter->areas[1]->set_left(position);
  }
}

void Area::set_right(int position) {
  this->width = position - this->left;
  if (this->splitter == NULL) return;
  if (!this->splitter->is_vertical) {
    this->splitter->areas[0]->set_right(position);
  }
  this->splitter->areas[1]->set_right(position);
}

void Area::set_top(int position) {
  int bottom = this->top + this->height;
  this->top = position;
  this->height = bottom - position;  // preserve bottom
  if (this->splitter == NULL) return;
  this->splitter->areas[0]->set_top(position);
  if (this->splitter->is_vertical) {
    this->splitter->areas[1]->set_top(position);
  }
}

void Area::set_bottom(int position) {
  this->height = position - this->top;
  if (this->splitter == NULL) return;
  this->splitter->areas[0]->set_bottom(position);
  if (this->splitter->is_vertical) {
    this->splitter->areas[1]->set_bottom(position);
  }
}

Rect Area_Splitter::get_rect() {
  Rect result = {};
  const int kSensitivity = 5;
  Area *area = this->areas[0];  // take one area for reference

  if (this->is_vertical) {
    result.left = this->position - kSensitivity;
    result.right = this->position + kSensitivity;
    result.top = area->top + kSensitivity;
    result.bottom = area->top + area->height - kSensitivity;
  } else {
    result.top = this->position - kSensitivity;
    result.bottom = this->position + kSensitivity;
    result.left = area->left + kSensitivity;
    result.right = area->left + area->width - kSensitivity;
  }

  return result;
}

bool Area_Splitter::is_mouse_over(v2i mouse) {
  return this->get_rect().is_within(mouse);
}

void Area_Splitter::move(v2i mouse) {
  Area *area1 = this->areas[0];
  Area *area2 = this->areas[1];
  int new_position = this->is_vertical ? mouse.x : mouse.y;

  if (this->position == new_position) return;
  this->position = new_position;

  if (this->is_vertical) {
    area1->set_right(new_position);
    area2->set_left(new_position);
  } else {
    area1->set_bottom(new_position);
    area2->set_top(new_position);
  }
}

Area *User_Interface::create_area(Rect rect, Area_Splitter *splitter = NULL) {
  assert(this->num_areas >= 0 && this->num_areas < EDITOR_MAX_AREA_COUNT);

  Area *area = &this->areas[this->num_areas];
  this->num_areas++;

  *area = {};
  area->set_rect(rect);
  area->splitter = splitter;

  return area;
}

Area_Splitter *User_Interface::_new_splitter(Area *area) {
  assert(this->num_splitters >= 0 &&
         this->num_splitters < EDITOR_MAX_AREA_COUNT);

  Area_Splitter *splitter = &this->splitters[this->num_splitters];
  area->splitter = splitter;
  this->num_splitters++;

  *splitter = {};
  splitter->parent_area = area;

  return splitter;
}

Area_Splitter *User_Interface::vertical_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area);
  splitter->is_vertical = true;
  splitter->position = area->left + position;

  // Create 2 areas
  Rect rect = area->get_rect();
  rect.right = rect.left + position;
  splitter->areas[0] = this->create_area(rect);

  rect = area->get_rect();
  rect.left = rect.left + position;
  splitter->areas[1] = this->create_area(rect);

  return splitter;
}

Area_Splitter *User_Interface::horizontal_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area);
  splitter->is_vertical = false;
  splitter->position = area->top + position;

  // Create 2 areas
  Rect rect = area->get_rect();
  rect.bottom = rect.top + position;
  splitter->areas[0] = this->create_area(rect);

  rect = area->get_rect();
  rect.top = rect.top + position;
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

    // TMP
    Area *area;
    Area_Splitter *splitter;
    area =
        ui->create_area({0, 0, g_state->kWindowWidth, g_state->kWindowHeight});
    splitter = ui->vertical_split(area, area->width / 2);
    area = splitter->areas[0];
    splitter = ui->horizontal_split(area, area->height / 3);
    splitter =
        ui->vertical_split(splitter->areas[1], splitter->areas[1]->width / 3);
    area = splitter->areas[1];
    splitter = ui->horizontal_split(area, area->height / 4);
    // area = splitter->areas[1];
    // splitter = ui->horizontal_split(area, 2 * area->height / 3);
  }

  User_Interface *ui = &g_state->UI;

  if (pixel_buffer->was_resized) {
    ui->areas[0].resize(pixel_buffer->width, pixel_buffer->height);
    pixel_buffer->was_resized = false;
  }

  // Drag splitters with a mouse
  for (int i = 0; i < ui->num_splitters; i++) {
    Area_Splitter *splitter = &ui->splitters[i];
    if (input->mouse_left && !splitter->being_moved &&
        splitter->is_mouse_over(input->mouse)) {
      splitter->being_moved = true;
    }
    if (!input->mouse_left) {
      splitter->being_moved = false;
    }
    if (splitter->being_moved) {
      splitter->move(input->mouse);
    }
  }

  // Clear
  memset(pixel_buffer->memory, 0,
         pixel_buffer->width * pixel_buffer->height * 4);

  assert(ui->num_areas > 0 && ui->num_areas < EDITOR_MAX_AREA_COUNT);
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
