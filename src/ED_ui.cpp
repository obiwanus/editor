#include "ED_ui.h"

inline u32 get_rgb_u32(v3 color) {
  assert(color.r >= 0 && color.r <= 1);
  assert(color.g >= 0 && color.g <= 1);
  assert(color.b >= 0 && color.b <= 1);

  u32 result = 0x00000000;
  u8 R = (u8)(color.r * 255);
  u8 G = (u8)(color.g * 255);
  u8 B = (u8)(color.b * 255);
  result = R << 16 | G << 8 | B;
  return result;
}

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2i point, u32 color) {
  int x = point.x;
  int y = point.y;

  if (x < 0 || x > pixel_buffer->width || y < 0 || y > pixel_buffer->height) {
    return;
  }
  // y = pixel_buffer->height - y;  // Origin in bottom-left
  u32 *pixel = (u32 *)pixel_buffer->memory + x + y * pixel_buffer->width;
  *pixel = color;
}

inline void draw_pixel(Pixel_Buffer *pixel_buffer, v2 point, u32 color) {
  // A v2 version
  v2i point_i = {(int)point.x, (int)point.y};
  draw_pixel(pixel_buffer, point_i, color);
}

void draw_line(Pixel_Buffer *pixel_buffer, v2i A, v2i B, u32 color) {
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
      draw_pixel(pixel_buffer, V2i(x, y), color);
    } else {
      draw_pixel(pixel_buffer, V2i(y, x), color);
    }
    error += sign * dy;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_line(Pixel_Buffer *pixel_buffer, v2 A, v2 B, u32 color) {
  v2i a = {(int)A.x, (int)A.y};
  v2i b = {(int)B.x, (int)B.y};
  draw_line(pixel_buffer, a, b, color);
}

void draw_rect(Pixel_Buffer *pixel_buffer, Rect rect, u32 color) {
  if (rect.left < 0) rect.left = 0;
  if (rect.top < 0) rect.top = 0;
  if (rect.right > pixel_buffer->width) rect.right = pixel_buffer->width;
  if (rect.bottom > pixel_buffer->height) rect.bottom = pixel_buffer->height;

  for (int x = rect.left; x < rect.right; x++) {
    for (int y = rect.top; y < rect.bottom; y++) {
      // Don't care about performance (yet)
      draw_pixel(pixel_buffer, V2i(x, y), color);
    }
  }
}

void draw_rect(Pixel_Buffer *pixel_buffer, Rect rect, v3 color) {
  draw_rect(pixel_buffer, rect, get_rgb_u32(color));
}

bool Rect::contains(v2i point) {
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

Rect Area::get_split_handle(int num) {
  Rect rect;
  assert(num == 0 || num == 1);
  const int kSquareSize = 15;
  rect = this->get_rect();
  if (num == 0) {
    rect.top = rect.bottom - kSquareSize;
    rect.right = rect.left + kSquareSize;
  } else {
    rect.left = rect.right - kSquareSize;
    rect.bottom = rect.top + kSquareSize;
  }
  return rect;
}

inline int Area::get_width() { return this->right - this->left; }

inline int Area::get_height() { return this->bottom - this->top; }

inline Rect Area::get_client_rect() {
  Rect result;

  result = this->get_rect();
  result.bottom = result.bottom - AREA_PANEL_HEIGHT;

  return result;
}

void Area::set_left(int position) {
  this->left = position;
  if (this->splitter == NULL) return;
  this->splitter->areas[0]->set_left(position);
  if (!this->splitter->is_vertical) {
    this->splitter->areas[1]->set_left(position);
  }
}

void Area::set_right(int position) {
  this->right = position;
  if (this->splitter == NULL) return;
  if (!this->splitter->is_vertical) {
    this->splitter->areas[0]->set_right(position);
  }
  this->splitter->areas[1]->set_right(position);
}

void Area::set_top(int position) {
  this->top = position;
  if (this->splitter == NULL) return;
  this->splitter->areas[0]->set_top(position);
  if (this->splitter->is_vertical) {
    this->splitter->areas[1]->set_top(position);
  }
}

void Area::set_bottom(int position) {
  this->bottom = position;
  if (this->splitter == NULL) return;
  this->splitter->areas[1]->set_bottom(position);
  if (this->splitter->is_vertical) {
    this->splitter->areas[0]->set_bottom(position);
  }
}

inline Rect Area::get_rect() {
  Rect rect;

  rect.left = this->left;
  rect.top = this->top;
  rect.right = this->right;
  rect.bottom = this->bottom;

  return rect;
}

inline void Area::set_rect(Rect rect) {
  this->left = rect.left;
  this->top = rect.top;
  this->right = rect.right;
  this->bottom = rect.bottom;
}

bool Area::mouse_over_split_handle(v2i mouse) {
  return this->get_split_handle(0).contains(mouse) ||
         this->get_split_handle(1).contains(mouse);
}

bool Area::is_visible() { return this->splitter == NULL; }

Rect Area_Splitter::get_rect() {
  Rect result = {};
  const int kSensitivity = 5;
  Area *area = this->areas[0];  // take one area for reference

  if (this->is_vertical) {
    result.left = this->position - kSensitivity;
    result.right = this->position + kSensitivity;
    result.top = area->top + kSensitivity;
    result.bottom = area->bottom - kSensitivity;
  } else {
    result.top = this->position - kSensitivity;
    result.bottom = this->position + kSensitivity;
    result.left = area->left + kSensitivity;
    result.right = area->right - kSensitivity;
  }

  return result;
}

bool Area_Splitter::is_mouse_over(v2i mouse) {
  return this->get_rect().contains(mouse);
}

void Area_Splitter::move(v2i mouse) {
  Area *area1 = this->areas[0];
  Area *area2 = this->areas[1];
  int new_position = this->is_vertical ? mouse.x : mouse.y;

  if (this->position == new_position) return;
  if (new_position < this->position_min) new_position = this->position_min;
  if (this->position_max < new_position) new_position = this->position_max;

  this->position = new_position;

  if (this->is_vertical) {
    area1->set_right(new_position - 1);
    area2->set_left(new_position + 2);
  } else {
    area1->set_bottom(new_position - 1);
    area2->set_top(new_position + 1);
  }
}

bool Area_Splitter::is_under(Area *area) {
  Area *parent = this->parent_area;
  while (parent != NULL) {
    if (parent == area) return true;
    parent = parent->parent_area;
  }
  return false;
}

void Pixel_Buffer::allocate(Program_Memory *program_memory) {
  *this = {};
  this->max_width = 3000;
  this->max_height = 3000;
  this->memory = program_memory->allocate(this->max_width * this->max_height *
                                          sizeof(u32));
}

Rect Pixel_Buffer::get_rect() {
  Rect result = {0, 0, this->width, this->height};
  return result;
}

Rect UI_Select::get_rect() {
  assert(this->parent_area != NULL);
  Rect rect;
  Rect bounds = {0, 0, this->parent_area->get_width(),
                 this->parent_area->get_height()};

  const int kSelectWidth = 100;
  const int kSelectHeight = 18;

  if (this->align_bottom) {
    rect.bottom = bounds.bottom - this->y;
    rect.top = rect.bottom - kSelectHeight;
  } else {
    rect.top = bounds.top + this->y;
    rect.bottom = rect.top + kSelectHeight;
  }
  if (this->align_right) {
    rect.right = bounds.right - this->x;
    rect.left = rect.right - kSelectWidth;
  } else {
    rect.left = bounds.left + this->x;
    rect.right = rect.left + kSelectWidth;
  }

  return rect;
}

Rect UI_Select::get_absolute_rect() {
  assert(this->parent_area != NULL);
  Rect rect = this->get_rect();

  rect.left += this->parent_area->left;
  rect.right += this->parent_area->left;
  rect.top += this->parent_area->top;
  rect.bottom += this->parent_area->top;

  return rect;
}

Area *User_Interface::create_area(Area *parent_area, Rect rect,
                                  Pixel_Buffer *draw_buffer) {
  assert(this->num_areas >= 0 && this->num_areas < EDITOR_MAX_AREA_COUNT - 1);

  Area *area = this->areas + this->num_areas;
  this->num_areas++;

  *area = {};
  area->set_rect(rect);
  area->parent_area = parent_area;
  area->editor_empty.area = area;
  area->editor_raytrace.area = area;
  if (parent_area != NULL) {
    area->editor_type = parent_area->editor_type;
  }

  // Create or set draw buffer
  if (draw_buffer != NULL) {
    area->draw_buffer = draw_buffer;
  } else {
    area->draw_buffer =
        (Pixel_Buffer *)this->memory->allocate(sizeof(Pixel_Buffer));
    area->draw_buffer->allocate(this->memory);
  }
  area->draw_buffer->width = area->get_width();
  area->draw_buffer->height = area->get_height();

  // Create a type selector
  UI_Select *select = this->selects + this->num_selects;
  this->num_selects++;
  *select = {};
  select->align_bottom = true;
  select->x = 20;
  select->y = 3;
  select->parent_area = area;

  return area;
}

Area_Splitter *User_Interface::_new_splitter(Area *area) {
  assert(this->num_splitters >= 0 &&
         this->num_splitters < EDITOR_MAX_AREA_COUNT);

  Area_Splitter *splitter = this->splitters + this->num_splitters;
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
  splitter->position = position;

  // Create 2 areas
  Rect rect = area->get_rect();
  rect.right = position;
  splitter->areas[0] = this->create_area(area, rect, area->draw_buffer);

  rect = area->get_rect();
  rect.left = position;
  splitter->areas[1] = this->create_area(area, rect);

  return splitter;
}

Area_Splitter *User_Interface::horizontal_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area);
  splitter->is_vertical = false;
  splitter->position = position;

  // Create 2 areas
  Rect rect = area->get_rect();
  rect.bottom = position;
  splitter->areas[0] = this->create_area(area, rect, area->draw_buffer);

  rect = area->get_rect();
  rect.top = position;
  splitter->areas[1] = this->create_area(area, rect);

  return splitter;
}

void User_Interface::set_movement_boundaries(Area_Splitter *splitter) {
  int position_min;
  int position_max;
  if (splitter->is_vertical) {
    position_min = splitter->parent_area->left;
    position_max = splitter->parent_area->right;
  } else {
    position_min = splitter->parent_area->top;
    position_max = splitter->parent_area->bottom;
  }

  // Look at all splitters which are under our parent area
  for (int i = 0; i < this->num_splitters; i++) {
    Area_Splitter *s = this->splitters + i;
    if (!s->is_under(splitter->parent_area) || s == splitter) continue;
    if (s->is_vertical != splitter->is_vertical) continue;

    if (position_min < s->position && s->position < splitter->position) {
      position_min = s->position;
    }
    if (splitter->position < s->position && s->position < position_max) {
      position_max = s->position;
    }
  }

  int kMargin = 15;
  if (!splitter->is_vertical) {
    kMargin = AREA_PANEL_HEIGHT;
  }
  splitter->position_max = position_max - kMargin;
  splitter->position_min = position_min + kMargin;
}

void User_Interface::resize_window(int new_width, int new_height) {
  if (this->num_areas <= 0) return;
  Area *main_area = &this->areas[0];
  int old_width = main_area->get_width();
  int old_height = main_area->get_height();

  main_area->right = new_width;
  main_area->bottom = new_height;
  r32 width_ratio = (r32)new_width / (r32)old_width;
  r32 height_ratio = (r32)new_height / (r32)old_height;

  // Go over every splitter and calculate new position
  for (int i = 0; i < this->num_splitters; i++) {
    Area_Splitter *splitter = &this->splitters[i];
    Area *area1 = splitter->areas[0];
    Area *area2 = splitter->areas[1];
    // Match the parent first
    Rect parent_rect = splitter->parent_area->get_rect();
    area1->set_rect(parent_rect);
    area2->set_rect(parent_rect);
    if (splitter->is_vertical) {
      splitter->position = (int)(splitter->position * width_ratio);
      area1->right = splitter->position - 1;
      area2->left = splitter->position + 2;
    } else {
      splitter->position = (int)(splitter->position * height_ratio);
      area1->bottom = splitter->position - 1;
      area2->top = splitter->position + 1;
    }
  }
}

Update_Result User_Interface::update_and_draw(Pixel_Buffer *pixel_buffer,
                                              User_Input *input) {
  Update_Result result = {};
  User_Interface *ui = this;

  if (pixel_buffer->was_resized) {
    ui->resize_window(pixel_buffer->width, pixel_buffer->height);
    pixel_buffer->was_resized = false;
  }

  // ----- Update ----------------------------------------------------

  if (input->mouse_left) {
    if (ui->area_being_split == NULL && ui->can_split_area &&
        ui->splitter_being_moved == NULL) {
      // See if we're splitting any area
      for (int i = 0; i < ui->num_areas; i++) {
        Area *area = ui->areas + i;
        if (area->is_visible() && area->mouse_over_split_handle(input->mouse)) {
          ui->area_being_split = area;
          ui->pointer_start = input->mouse;
          break;
        }
      }
      // Prevent splitting areas by swipe
      ui->can_split_area = false;
    }

    if (ui->area_being_split != NULL) {
      // See if mouse has moved enough to finish the split
      if (ui->area_being_split->get_rect().contains(input->mouse)) {
        const int kMargin = 25;
        v2i distance = ui->pointer_start - input->mouse;
        distance.x = abs(distance.x);
        distance.y = abs(distance.y);
        if (distance.x > kMargin || distance.y > kMargin) {
          Area_Splitter *splitter;
          if (distance.x > distance.y) {
            splitter = ui->vertical_split(ui->area_being_split, input->mouse.x);
          } else {
            splitter =
                ui->horizontal_split(ui->area_being_split, input->mouse.y);
          }
          ui->splitter_being_moved = splitter;
          ui->set_movement_boundaries(splitter);
          ui->area_being_split = NULL;
          ui->can_pick_splitter = false;
          ui->can_split_area = false;
        }
      } else {
        // Stop splitting if mouse cursor is outside of the area
        ui->area_being_split = NULL;
        ui->can_pick_splitter = false;
      }
    } else {
      // Only look at splitters if areas are not being split
      if (ui->can_pick_splitter && ui->splitter_being_moved == NULL) {
        for (int i = 0; i < ui->num_splitters; i++) {
          Area_Splitter *splitter = ui->splitters + i;
          if (input->mouse_left && ui->can_pick_splitter &&
              splitter->is_mouse_over(input->mouse)) {
            ui->splitter_being_moved = splitter;
            ui->set_movement_boundaries(splitter);
            ui->can_pick_splitter = false;
          }
        }
        // Prevent dragging splitters by swipe
        ui->can_pick_splitter = false;
      }
      // Move splitter if we can
      if (ui->splitter_being_moved != NULL) {
        ui->splitter_being_moved->move(input->mouse);
      }
    }
  } else {
    // Mouse button released or not pressed
    // TODO: be able to tell which is which maybe?
    ui->area_being_split = NULL;
    ui->splitter_being_moved = NULL;
    ui->can_pick_splitter = true;
    ui->can_split_area = true;
  }

  // ------- Draw area contents -------------------------------------------

  // Potentially will be in separate threads
  ui->draw_areas(NULL);

  // Draw panels
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas + i;
    if (!area->is_visible()) continue;

    // Draw panels
    Rect panel_rect = area->draw_buffer->get_rect();
    panel_rect.top = panel_rect.bottom - AREA_PANEL_HEIGHT;
    draw_rect(area->draw_buffer, panel_rect, 0x00686868);
  }

  // Draw ui selects
  for (int i = 0; i < ui->num_selects; i++) {
    UI_Select *select = ui->selects + i;
    if (select->parent_area && !select->parent_area->is_visible()) continue;

    // Update
    bool mouse_over = select->parent_area->get_rect().contains(input->mouse) &&
                      select->get_absolute_rect().contains(input->mouse);
    if (mouse_over) {
      select->highlighted = true;
    } else {
      select->highlighted = false;
    }

    // Draw
    Rect rect = select->get_rect();
    if (select->highlighted) {
      draw_rect(select->parent_area->draw_buffer, rect, 0x00222222);
    } else {
      draw_rect(select->parent_area->draw_buffer, rect, 0x00111111);
    }
  }

  // Copy area buffers into the main buffer
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas + i;
    if (!area->is_visible()) continue;
    Rect client_rect = area->get_client_rect();
    Pixel_Buffer *src_buffer = area->draw_buffer;
    for (int y = 0; y < src_buffer->height; y++) {
      for (int x = 0; x < src_buffer->width; x++) {
        u32 *pixel_src = (u32 *)src_buffer->memory + x + y * src_buffer->width;
        u32 *pixel_dest = (u32 *)pixel_buffer->memory + (client_rect.left + x) +
                          (client_rect.top + y) * pixel_buffer->width;
        *pixel_dest = *pixel_src;
      }
    }
  }

  // TODO: optimize multiple loops.
  // or maybe use an iterator if we need to keep them?

  // ------ Draw UI elements ----------------------------------------------

  // Draw splitters
  for (int i = 0; i < ui->num_splitters; i++) {
    Area_Splitter *splitter = ui->splitters + i;
    // draw_rect(pixel_buffer, splitter->get_rect(), {1,1,1});
    Area *area = splitter->parent_area;
    int left, top, right, bottom;
    if (splitter->is_vertical) {
      left = splitter->position;
      top = area->top;
      bottom = area->bottom;
      draw_line(pixel_buffer, V2i(left - 1, top), V2i(left - 1, bottom),
                0x00323232);
      draw_line(pixel_buffer, V2i(left, top), V2i(left, bottom), 0x00000000);
      draw_line(pixel_buffer, V2i(left + 1, top), V2i(left + 1, bottom),
                0x00505050);
    } else {
      top = splitter->position;
      left = area->left;
      right = area->right;
      draw_line(pixel_buffer, V2i(left, top - 1), V2i(right, top - 1),
                0x00000000);
      draw_line(pixel_buffer, V2i(left, top), V2i(right, top), 0x00505050);
    }
  }

  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas + i;
    if (!area->is_visible()) continue;

    // Draw split handles
    draw_rect(pixel_buffer, area->get_split_handle(0), {0.5f, 0.5f, 0.5f});
    draw_rect(pixel_buffer, area->get_split_handle(1), {0.5f, 0.5f, 0.5f});
  }

  // -- Cursors ---------------

  // Splitter resize cursor
  for (int i = 0; i < ui->num_splitters; i++) {
    Area_Splitter *splitter = ui->splitters + i;
    if (splitter->get_rect().contains(input->mouse)) {
      result.cursor = splitter->is_vertical ? Cursor_Type_Resize_Horiz
                                            : Cursor_Type_Resize_Vert;
    }
  }

  // Area split cursor
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas + i;
    if (area->mouse_over_split_handle(input->mouse)) {
      result.cursor = Cursor_Type_Cross;
    }
  }

  // Set the right cursors on actual resize or split
  if (ui->area_being_split != NULL) {
    result.cursor = Cursor_Type_Cross;
  } else if (ui->splitter_being_moved != NULL) {
    result.cursor = ui->splitter_being_moved->is_vertical
                        ? Cursor_Type_Resize_Horiz
                        : Cursor_Type_Resize_Vert;
  }

  return result;
}

void User_Interface::draw_areas(Ray_Tracer *rt) {
  // Draw areas
  for (int i = 0; i < this->num_areas; i++) {
    Area *area = this->areas + i;
    if (!area->is_visible()) continue;  // ignore wrapper areas
    area->draw_buffer->width = area->get_width();
    area->draw_buffer->height = area->get_height();

    // Draw editor contents
    switch (area->editor_type) {
      case Area_Editor_Type_Empty: {
        area->editor_empty.draw();
      } break;

      case Area_Editor_Type_Raytrace: {
        area->editor_raytrace.draw(rt);
      } break;

      default: { assert(!"Unknown editor type"); } break;
    }
  }
}

void Editor_Empty::draw() {
  draw_rect(this->area->draw_buffer, this->area->draw_buffer->get_rect(), {0});
}

void Editor_Raytrace::draw(Ray_Tracer *rt) {
  // TODO: ray tracer should probably be member and not
  // a function parameter

  if (rt == NULL) {
    draw_rect(this->area->draw_buffer, this->area->draw_buffer->get_rect(),
              0x00123123);
    return;
  }

  RayCamera *camera = &rt->camera;
  LightSource *lights = rt->lights;
  RayObject **ray_objects = rt->ray_objects;

  Rect client_rect = this->area->get_client_rect();

  v2i pixel_count = {client_rect.right - client_rect.left,
                     client_rect.bottom - client_rect.top};

  // TODO: this is wrong but tmp.
  // Ideally a raytrace view should have its own camera,
  // but this is probably not going to happen anyway
  camera->left = -pixel_count.x / 2;
  camera->right = pixel_count.x / 2;
  camera->top = pixel_count.y / 2;
  camera->bottom = -pixel_count.y / 2;

  for (int x = 0; x < pixel_count.x; x++) {
    for (int y = 0; y < pixel_count.y; y++) {
      Ray ray = camera->get_ray_through_pixel(x, y, pixel_count);

      const v3 ambient_color = {0.2f, 0.2f, 0.2f};
      const r32 ambient_light_intensity = 0.3f;

      v3 color = ambient_color * ambient_light_intensity;

      color += ray.get_color(rt, 0, rt->kMaxRecursion);

      // Crop
      for (int i = 0; i < 3; i++) {
        if (color.E[i] > 1) {
          color.E[i] = 1;
        }
        if (color.E[i] < 0) {
          color.E[i] = 0;
        }
      }

      draw_pixel(this->area->draw_buffer, V2i(x, y), get_rgb_u32(color));
    }
  }
}
