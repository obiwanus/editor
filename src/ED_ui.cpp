#include <stdlib.h>
#include <string.h>
#include "include/stb_stretchy_buffer.h"
#include "ED_core.h"
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
  // TODO: replace this with a fast version
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

v2i Rect::projected(v2i point) {
  v2i result = point;

  result.x -= this->left;
  result.y -= this->top;

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
  Rect rect = this->get_rect();
  assert(num == 0 || num == 1);
  const int kSquareSize = 15;
  if (num == 0) {
    rect.top = rect.bottom - kSquareSize;
    rect.right = rect.left + kSquareSize;
  } else {
    rect.left = rect.right - kSquareSize;
    rect.bottom = rect.top + kSquareSize;
  }
  return rect;
}

Rect Area::get_delete_button() {
  Rect rect = this->get_rect();
  const int kSquareSize = 10;
  rect.right = rect.left + kSquareSize;
  rect.bottom = rect.top + kSquareSize;

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

void Area::deallocate() {
  if (this->draw_buffer != NULL) {
    free(this->draw_buffer->memory);
    free(this->draw_buffer);
  }
  free(this);
}

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

void Pixel_Buffer::allocate() {
  *this = {};
  this->max_width = 3000;
  this->max_height = 3000;
  this->memory = malloc(this->max_width * this->max_height * sizeof(u32));
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

Area *User_Interface::create_area(Area *parent_area, Rect rect) {
  // TODO: I don't want to allocate things randomly on the heap,
  // so later it'd be good to have a pool allocator for this
  // possibly with the ability to remove elements
  Area *area = (Area *)malloc(sizeof(*area));
  if (sb_count(this->areas) > this->num_areas) {
    this->areas[this->num_areas] = area;
  } else {
    sb_push(Area **, this->areas, area);
  }
  this->num_areas++;

  *area = {};
  area->set_rect(rect);
  area->parent_area = parent_area;
  area->editor_3dview.area = area;
  area->editor_raytrace.area = area;
  if (parent_area != NULL) {
    area->editor_type = parent_area->editor_type;
  }

  // Create draw buffer
  area->draw_buffer = (Pixel_Buffer *)malloc(sizeof(Pixel_Buffer));
  area->draw_buffer->allocate();
  area->draw_buffer->width = area->get_width();
  area->draw_buffer->height = area->get_height();

  return area;
}

void User_Interface::remove_area(Area *area) {
  Area *sister_area = NULL;
  Area *parent_area = area->parent_area;
  for (int i = 0; i < 2; i++) {
    if (parent_area->splitter->areas[i] != area) {
      sister_area = parent_area->splitter->areas[i];
    }
  }
  assert(sister_area != NULL);

  // Remove splitter
  {
    int splitter_id = -1;
    for (int i = 0; i < this->num_splitters; i++) {
      Area_Splitter *splitter = this->splitters[i];
      if (parent_area->splitter == splitter) {
        splitter_id = i;
        break;
      }
    }
    assert(0 <= splitter_id && splitter_id < this->num_splitters);
    this->num_splitters--;
    free(this->splitters[splitter_id]);
    this->splitters[splitter_id] = this->splitters[this->num_splitters];
    this->splitters[this->num_splitters] = {};
    parent_area->splitter = NULL;
  }

  // Find type selects
  UI_Select *area_select = NULL;
  UI_Select *sister_select = NULL;
  for (int i = 0; i < this->num_selects; i++) {
    UI_Select *select = this->selects[i];
    if (select->parent_area == area) {
      area_select = select;
    } else if (select->parent_area == sister_area) {
      sister_select = select;
    }
  }
  assert(area_select != NULL);

  parent_area->editor_type = sister_area->editor_type;
  if (sister_select != NULL) {
    sister_select->parent_area = parent_area;
  }
  if (sister_area->splitter != NULL) {
    parent_area->splitter = sister_area->splitter;
    parent_area->splitter->parent_area = parent_area;
    for (int j = 0; j < 2; j++) {
      parent_area->splitter->areas[j]->parent_area = parent_area;
    }
  }

  // Delete area select
  {
    int select_id = -1;
    for (int i = 0; i < this->num_selects; i++) {
      UI_Select *select = this->selects[i];
      if (area_select == select) {
        select_id = i;
        break;
      }
    }
    assert(0 <= select_id && select_id < this->num_selects);
    this->num_selects--;
    free(this->selects[select_id]);
    this->selects[select_id] = this->selects[this->num_selects];
    this->selects[this->num_selects] = {};
  }

  // Remove areas
  {
    int area_id = -1;
    int sister_area_id = -1;
    for (int i = 0; i < this->num_areas; i++) {
      Area *a = this->areas[i];
      if (a == area) {
        area_id = i;
      }
      if (a == sister_area) {
        sister_area_id = i;
      }
    }
    assert(0 <= area_id && area_id < this->num_areas);
    assert(0 <= sister_area_id && sister_area_id < this->num_areas);
    int edge = this->num_areas - 1;
    if (area_id < edge) {
      this->areas[area_id]->deallocate();
      this->areas[area_id] = this->areas[edge];
      this->areas[edge] = {};
      if (sister_area_id == edge) {
        // Make sure we overwrite it later
        sister_area_id = area_id;
      }
    }
    edge--;
    if (sister_area_id < edge) {
      this->areas[sister_area_id]->deallocate();
      this->areas[sister_area_id] = this->areas[edge];
      this->areas[edge] = {};
    }
    this->num_areas -= 2;
  }

  // Adjust the rects
  Rect parent_rect = parent_area->get_rect();
  parent_area->set_left(parent_rect.left);
  parent_area->set_right(parent_rect.right);
  parent_area->set_top(parent_rect.top);
  parent_area->set_bottom(parent_rect.bottom);
}

Area_Splitter *User_Interface::_new_splitter(Area *area) {
  Area_Splitter *splitter = (Area_Splitter *)malloc(sizeof(*splitter));
  if (sb_count(this->splitters) > this->num_splitters) {
    this->splitters[this->num_splitters] = splitter;
  } else {
    sb_push(Area_Splitter **, this->splitters, splitter);
  }
  area->splitter = splitter;
  this->num_splitters++;

  *splitter = {};
  splitter->parent_area = area;

  return splitter;
}

UI_Select *User_Interface::new_type_selector(Area *area) {
  UI_Select *select = (UI_Select *)malloc(sizeof(*select));
  if (sb_count(this->selects) > this->num_selects) {
    this->selects[this->num_selects] = select;
  } else {
    sb_push(UI_Select **, this->selects, select);
  }
  this->num_selects++;
  *select = {};
  select->align_bottom = true;
  select->x = 20;
  select->y = 3;
  select->option_count = Area_Editor_Type__COUNT;
  select->option_height = 20;
  select->parent_area = area;
  select->option_selected = area->editor_type;

  return select;
}

void User_Interface::_split_type_selectors(Area *area, Area_Splitter *splitter,
                                           bool is_vertical) {
  // Find old select for this area
  UI_Select *old_select = NULL;
  for (int i = 0; i < this->num_selects; i++) {
    UI_Select *select = this->selects[i];
    if (select->parent_area == area) {
      old_select = select;
      break;
    }
  }
  assert(old_select != NULL);

  int position = splitter->position;
  Rect rect = area->get_rect();
  int bigger_area = 0;
  int smaller_area = 1;
  if ((is_vertical && (position - rect.left < rect.right - position)) ||
      (!is_vertical && (position - rect.top < rect.bottom - position))) {
    bigger_area = 1;
    smaller_area = 0;
  }

  // Old select goes to the bigger area
  old_select->parent_area = splitter->areas[bigger_area];

  // The other gets a new select
  this->new_type_selector(splitter->areas[smaller_area]);
}

Area_Splitter *User_Interface::vertical_split(Area *area, int position) {
  // Create splitter
  Area_Splitter *splitter = this->_new_splitter(area);
  splitter->is_vertical = true;
  splitter->position = position;

  // Create 2 areas
  Rect rect = area->get_rect();
  rect.right = position;
  splitter->areas[0] = this->create_area(area, rect);

  rect = area->get_rect();
  rect.left = position;
  splitter->areas[1] = this->create_area(area, rect);

  this->_split_type_selectors(area, splitter, true);

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
  splitter->areas[0] = this->create_area(area, rect);

  rect = area->get_rect();
  rect.top = position;
  splitter->areas[1] = this->create_area(area, rect);

  this->_split_type_selectors(area, splitter, false);

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
    Area_Splitter *s = this->splitters[i];
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

void reposition_splitter(Area *area, r32 width_ratio, r32 height_ratio) {
  Area_Splitter *splitter = area->splitter;
  if (splitter == NULL) return;
  assert(splitter->parent_area == area);

  Area *area1 = splitter->areas[0];
  Area *area2 = splitter->areas[1];
  // Match the parent first
  Rect parent_rect = area->get_rect();
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
  reposition_splitter(area1, width_ratio, height_ratio);
  reposition_splitter(area2, width_ratio, height_ratio);
}

void User_Interface::resize_window(int new_width, int new_height) {
  if (this->num_areas <= 0) return;
  Area *main_area = this->areas[0];
  int old_width = main_area->get_width();
  int old_height = main_area->get_height();

  main_area->right = new_width;
  main_area->bottom = new_height;
  r32 width_ratio = (r32)new_width / (r32)old_width;
  r32 height_ratio = (r32)new_height / (r32)old_height;

  // Recursively reposition all splitters
  reposition_splitter(main_area, width_ratio, height_ratio);
}

Update_Result User_Interface::update_and_draw(Pixel_Buffer *pixel_buffer,
                                              User_Input *input, Model model) {
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
        Area *area = ui->areas[i];
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
          Area_Splitter *splitter = ui->splitters[i];
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

    // Maybe we're deleting an area?
    if (ui->can_delete_area == 0) {
      for (int i = 1; i < ui->num_areas; i++) {
        // Note we're not considering area 0
        Area *area = ui->areas[i];
        if (!area->is_visible()) continue;
        if (area->get_delete_button().contains(input->mouse)) {
          ui->can_delete_area = i;
          break;
        }
      }
    }
  } else {
    // Mouse button released or not pressed
    // TODO: be able to tell which is which maybe?
    ui->area_being_split = NULL;
    ui->splitter_being_moved = NULL;
    ui->can_pick_splitter = true;
    ui->can_split_area = true;

    // NOTE: if we need to check something for all areas
    // every frame, consider putting it here

    // If we've released LMB we may be deleting an area
    for (int i = 1; i < ui->num_areas; i++) {
      // Note we're not considering area 0
      Area *area = ui->areas[i];
      if (!area->is_visible()) continue;
      if (area->get_delete_button().contains(input->mouse)) {
        if (ui->can_delete_area == i) {
          ui->remove_area(area);
        }
        break;
      }
    }
    ui->can_delete_area = 0;
  }

  // ------- Draw area contents -------------------------------------------

  // Potentially will be in separate threads
  ui->draw_areas(NULL, model);

  // Draw panels
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas[i];
    if (!area->is_visible()) continue;

    // Draw panels
    Rect panel_rect = area->draw_buffer->get_rect();
    panel_rect.top = panel_rect.bottom - AREA_PANEL_HEIGHT;
    draw_rect(area->draw_buffer, panel_rect, 0x00686868);
  }

  bool mouse_over_any_select = false;

  // Draw ui selects
  for (int i = 0; i < ui->num_selects; i++) {
    UI_Select *select = ui->selects[i];
    if (select->parent_area && !select->parent_area->is_visible()) continue;

    Rect select_rect = select->get_rect();
    Rect parent_area_rect = select->parent_area->get_rect();
    Rect all_options_rect;
    int kMargin = 1;
    all_options_rect.left = select_rect.left - kMargin;
    all_options_rect.right = select_rect.right + 30 + kMargin;
    all_options_rect.bottom = select_rect.bottom + kMargin;
    all_options_rect.top = select_rect.top -
                           select->option_count * (select->option_height + 1) -
                           kMargin + 1;

    // Update
    bool mouse_in_area = parent_area_rect.contains(input->mouse);
    bool mouse_over_select =
        mouse_in_area &&
        select_rect.contains(parent_area_rect.projected(input->mouse));
    bool mouse_over_options =
        mouse_in_area &&
        all_options_rect.contains(parent_area_rect.projected(input->mouse));

    if (mouse_over_select) {
      mouse_over_any_select = true;
    }

    if (mouse_over_select && input->mouse_left && ui->can_pick_select) {
      select->open = true;
      ui->can_pick_select = false;
    }

    if (select->open && (!mouse_in_area || !mouse_over_options)) {
      select->open = false;
      ui->can_pick_select = false;
    }

    if (mouse_over_select && !input->mouse_left) {
      ui->can_pick_select = true;
    }

    if (mouse_over_select) {
      select->highlighted = true;
    } else if (!select->open) {
      select->highlighted = false;
    }

    const u32 colors[10] = {0x00000000, 0x00123123};

    // Draw
    if (select->highlighted) {
      draw_rect(select->parent_area->draw_buffer, select_rect, 0x00222222);
    } else {
      draw_rect(select->parent_area->draw_buffer, select_rect,
                colors[select->option_selected]);
    }

    if (select->open) {
      Rect options_border = all_options_rect;
      options_border.bottom = select_rect.top + kMargin;

      // Draw all options rect first
      draw_rect(select->parent_area->draw_buffer, options_border, 0x00686868);

      int bottom = select_rect.top;
      for (int opt = 0; opt < select->option_count; opt++) {
        Rect option;
        option.bottom = bottom;
        option.left = select_rect.left;
        option.right = select_rect.right + 30;
        option.top = option.bottom - select->option_height;
        bool mouse_over_option =
            mouse_over_options &&
            option.contains(parent_area_rect.projected(input->mouse));
        u32 color = colors[opt];
        if (mouse_over_option) {
          color += 0x00121212;  // highlight
        }
        draw_rect(select->parent_area->draw_buffer, option, color);
        bottom -= select->option_height + 1;

        // Select the option on click
        if (mouse_over_option && input->mouse_left && ui->can_pick_select) {
          select->option_selected = opt;
          select->open = false;

          // TODO: if we ever generalize this, this bit will have to change
          select->parent_area->editor_type = (Area_Editor_Type)opt;
        }

        if (mouse_over_option && !input->mouse_left) {
          ui->can_pick_select = true;
          mouse_over_any_select = true;
        }
      }
    }
  }

  // If mouse has left a select, we can't pick them anymore
  if (!mouse_over_any_select) {
    ui->can_pick_select = false;
  }

  // Copy area buffers into the main buffer
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas[i];
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
    Area_Splitter *splitter = ui->splitters[i];
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
    Area *area = ui->areas[i];
    if (!area->is_visible()) continue;

    // Draw split handles
    draw_rect(pixel_buffer, area->get_split_handle(0), {0.5f, 0.5f, 0.5f});
    draw_rect(pixel_buffer, area->get_split_handle(1), {0.5f, 0.5f, 0.5f});

    // Draw delete buttons
    if (i > 0) {
      // Can't delete area 0
      draw_rect(pixel_buffer, area->get_delete_button(), {0.4f, 0.1f, 0.1f});
    }
  }

  // -- Cursors ---------------

  // Splitter resize cursor
  for (int i = 0; i < ui->num_splitters; i++) {
    Area_Splitter *splitter = ui->splitters[i];
    if (splitter->get_rect().contains(input->mouse)) {
      result.cursor = splitter->is_vertical ? Cursor_Type_Resize_Horiz
                                            : Cursor_Type_Resize_Vert;
    }
  }

  // Area split cursor
  for (int i = 0; i < ui->num_areas; i++) {
    Area *area = ui->areas[i];
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

void User_Interface::draw_areas(Ray_Tracer *rt, Model model) {
  // Draw areas
  for (int i = 0; i < this->num_areas; i++) {
    Area *area = this->areas[i];
    if (!area->is_visible()) continue;  // ignore wrapper areas
    area->draw_buffer->width = area->get_width();
    area->draw_buffer->height = area->get_height();

    // Draw editor contents
    switch (area->editor_type) {
      case Area_Editor_Type_3DView: {
        if (!area->editor_3dview.is_drawn) {
          area->editor_3dview.draw(model);
          // area->editor_3dview.is_drawn = true;
        }
      } break;

      case Area_Editor_Type_Raytrace: {
        area->editor_raytrace.draw(rt);
      } break;

      default: { assert(!"Unknown editor type"); } break;
    }
  }
}

void Editor_3DView::draw(Model model) {
  Pixel_Buffer *buffer = this->area->draw_buffer;
  // draw_rect(buffer, buffer->get_rect(), {0});
  memset(buffer->memory, 0, buffer->width * buffer->height * sizeof(u32));
  u32 color = 0x00123123;

  r32 scale = (r32)buffer->height / 2.0f;
  // clang-format off
  m4x4 ScreenTransform = {
    scale, 0,     0,     (r32)buffer->width / 2,
    0,     scale, 0,     (r32)buffer->height / 2,
    0,     0,     scale, 0,
    0,     0,     0,     1,
  };
  // clang-format on

  for (int i = 0; i < sb_count(model.faces); i++) {
    Face face = model.faces[i];
    for (int j = 0; j < 3; j ++) {
      v3 vert1 = ScreenTransform * model.vertices[face.v_ids[j] - 1];
      v3 vert2 = ScreenTransform * model.vertices[face.v_ids[(j + 1) % 3] - 1];

      draw_line(buffer, V2i(vert1.x, vert1.y), V2i(vert2.x, vert2.y), color);
    }
  }
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
