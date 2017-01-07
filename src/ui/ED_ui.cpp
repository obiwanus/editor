
Rect Area::get_split_handle(int num) {
  Rect rect = {};
  assert(num == 0 || num == 1);
  const int kSquareSize = 15;
  if (num == 0) {
    rect.top = kSquareSize;
    rect.right = kSquareSize;
  } else {
    int area_width = this->get_width();
    int area_height = this->get_height();
    rect.left = area_width - kSquareSize;
    rect.right = area_width;
    rect.top = area_height;
    rect.bottom = area_height - kSquareSize;
  }
  return rect;
}

Rect Area::get_delete_button() {
  Rect rect;
  const int kSquareSize = 10;
  rect.left = 0;
  rect.top = this->get_height();
  rect.right = kSquareSize;
  rect.bottom = rect.top - kSquareSize;

  return rect;
}

inline int Area::get_width() { return this->right - this->left; }

inline int Area::get_height() { return this->top - this->bottom; }

inline Rect Area::get_client_rect() {
  Rect result;

  result = this->get_rect();
  result.bottom = result.bottom - Area::kPanelHeight;

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

void Area::reposition_splitter(r32 width_ratio, r32 height_ratio) {
  if (splitter == NULL) return;
  assert(splitter->parent_area == this);

  Area *area1 = splitter->areas[0];
  Area *area2 = splitter->areas[1];
  // Match the parent first
  Rect parent_rect = this->get_rect();
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
  area1->reposition_splitter(width_ratio, height_ratio);
  area2->reposition_splitter(width_ratio, height_ratio);
}

bool Area::mouse_over_split_handle(v2i mouse) {
  mouse = this->get_rect().projected(mouse);
  return this->get_split_handle(0).contains(mouse) ||
         this->get_split_handle(1).contains(mouse);
}

bool Area::mouse_over_delete_button(v2i mouse) {
  return this->get_delete_button().contains(this->get_rect().projected(mouse));
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
    result.top = this->position + kSensitivity;
    result.bottom = this->position - kSensitivity;
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
    area1->set_bottom(new_position + 1);
    area2->set_top(new_position - 1);
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

Rect UI_Select::get_rect() {
  // Strange that I was aiming at generalisation but made
  // this function extremely specific to type selects
  assert(this->parent_area != NULL);
  Rect rect = {};
  int area_width = this->parent_area->get_width();
  int area_height = this->parent_area->get_height();

  const int kSelectWidth = 100;
  const int kSelectHeight = 18;

  if (this->align_bottom) {
    rect.top = this->y + kSelectHeight;
    rect.bottom = this->y;
  } else {
    rect.top = area_height - this->y;
    rect.bottom = rect.top - kSelectHeight;
  }
  if (this->align_right) {
    rect.right = area_width - this->x;
    rect.left = rect.right - kSelectWidth;
  } else {
    rect.left = this->x;
    rect.right = rect.left + kSelectWidth;
  }

  return rect;
}

void UI_Select::update_and_draw(User_Input *input) {
  UI_Select *select = this;
  assert(select->parent_area != NULL);
  Area *area = select->parent_area;

  Rect area_rect = select->parent_area->get_rect();
  Rect select_rect = select->get_rect();

  // Draw unhighlighted no matter what
  const u32 colors[10] = {0x00000000, 0x00123123};
  draw_rect(area, select_rect, colors[select->option_selected]);

  // If mouse cursor is not in the area, don't interact with select
  if (!area_rect.contains(input->mouse)) {
    select->open = false;
    return;
  }

  const int kMargin = 1;
  Rect all_options_rect;
  all_options_rect.left = select_rect.left - kMargin;
  all_options_rect.right = select_rect.right + 30 + kMargin;
  all_options_rect.bottom = select_rect.bottom - kMargin;
  all_options_rect.top = select_rect.top +
                         select->option_count * (select->option_height + 1) +
                         kMargin - 1;

  // Update
  v2i mouse = area_rect.projected(input->mouse);
  bool mouse_over_select = select_rect.contains(mouse);
  bool mouse_over_options = all_options_rect.contains(mouse);

  if (mouse_over_select && input->button_went_down(IB_mouse_left)) {
    select->open = true;
  }

  if (select->open && !mouse_over_options) {
    select->open = false;
  }

  if (mouse_over_select || select->open) {
    // Draw highlighted
    draw_rect(area, select_rect, 0x00222222);
  }

  if (select->open) {
    Rect options_border = all_options_rect;
    options_border.bottom = select_rect.top + kMargin;

    // Draw all options rect first
    draw_rect(area, options_border, 0x00686868);

    int bottom = select_rect.top;
    for (int opt = 0; opt < select->option_count; opt++) {
      Rect option;
      option.bottom = bottom;
      option.left = select_rect.left;
      option.right = select_rect.right + 30;
      option.top = option.bottom + select->option_height;
      bool mouse_over_option = mouse_over_options && option.contains(mouse);
      u32 color = colors[opt];
      if (mouse_over_option) {
        color += 0x00121212;  // highlight
      }
      draw_rect(area, option, color);
      bottom += select->option_height + 1;

      // Select the option on click
      if (mouse_over_option && input->button_went_down(IB_mouse_left)) {
        select->option_selected = opt;
        select->open = false;

        // TODO: if we ever generalize this, this bit will have to change
        select->parent_area->editor_type = (Area_Editor_Type)opt;
      }
    }
  }
}

Area *User_Interface::create_area(Area *parent_area, Rect rect) {
  // TODO: I don't want to allocate things randomly on the heap,
  // so later it'd be good to have a pool allocator for this
  // possibly with the ability to remove elements

  // Create area
  Area *area = (Area *)malloc(sizeof(*area));
  {
    if (sb_count(this->areas) > this->num_areas) {
      this->areas[this->num_areas] = area;
    } else {
      sb_push(this->areas, area);
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
  }

  area->buffer = this->buffer;

  // Init type selector
  {
    UI_Select *select = &area->type_select;
    *select = {};
    select->align_bottom = true;
    select->x = 20;
    select->y = 4;
    select->option_count = Area_Editor_Type__COUNT;
    select->option_height = 20;
    select->parent_area = area;
    if (parent_area != NULL) {
      select->option_selected = area->editor_type;
    } else {
      // Default editor type
      select->option_selected = Area_Editor_Type_3DView;
    }
  }

  // Init camera for 3d view (or copy from parent)
  if (parent_area != NULL) {
    area->editor_3dview.camera = parent_area->editor_3dview.camera;
  } else {
    Camera *camera = &area->editor_3dview.camera;
    camera->position = V3(0, 1, 3);
    camera->up = V3(0, 1, 0);
    camera->look_at(V3(0, 0, 0));
  }

  return area;
}

void User_Interface::remove_area(Area *area) {
  Area *sister_area = NULL;
  Area *parent_area = area->parent_area;
  for (int i = 0; i < 2; ++i) {
    if (parent_area->splitter->areas[i] != area) {
      sister_area = parent_area->splitter->areas[i];
    }
  }
  assert(sister_area != NULL);

  // Copy data from sister area
  parent_area->editor_3dview.camera = sister_area->editor_3dview.camera;

  // Remove splitter
  {
    int splitter_id = -1;
    for (int i = 0; i < this->num_splitters; ++i) {
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

  parent_area->editor_type = sister_area->editor_type;
  if (sister_area->splitter != NULL) {
    parent_area->splitter = sister_area->splitter;
    parent_area->splitter->parent_area = parent_area;
    for (int j = 0; j < 2; j++) {
      parent_area->splitter->areas[j]->parent_area = parent_area;
    }
  }

  // Remove areas
  {
    int area_id = -1;
    int sister_area_id = -1;
    for (int i = 0; i < this->num_areas; ++i) {
      Area *a = this->areas[i];
      if (a == area) {
        area_id = i;
      }
      if (a == sister_area) {
        sister_area_id = i;
      }
    }

    free(area);
    free(sister_area);

    assert(0 <= area_id && area_id < this->num_areas);
    assert(0 <= sister_area_id && sister_area_id < this->num_areas);
    int edge = this->num_areas - 1;
    if (area_id < edge) {
      this->areas[area_id] = this->areas[edge];
      this->areas[edge] = {};
      if (sister_area_id == edge) {
        // Make sure we overwrite it later
        sister_area_id = area_id;
      }
    }
    edge--;
    if (sister_area_id < edge) {
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

Area_Splitter *User_Interface::split_area(Area *area, v2i mouse,
                                          bool is_vertical) {
  // Create splitter
  Area_Splitter *splitter;
  {
    splitter = (Area_Splitter *)malloc(sizeof(*splitter));
    if (sb_count(this->splitters) > this->num_splitters) {
      this->splitters[this->num_splitters] = splitter;
    } else {
      sb_push(this->splitters, splitter);
    }
    area->splitter = splitter;
    this->num_splitters++;

    *splitter = {};
    splitter->parent_area = area;
    splitter->is_vertical = is_vertical;
    splitter->position = is_vertical ? mouse.x : mouse.y;
  }

  // Create 2 areas
  {
    Rect rect1, rect2;
    rect1 = rect2 = area->get_rect();
    if (is_vertical) {
      rect1.right = mouse.x;
      rect2.left = mouse.x;
    } else {
      rect1.bottom = mouse.y;
      rect2.top = mouse.y;
    }
    splitter->areas[0] = this->create_area(area, rect1);
    splitter->areas[1] = this->create_area(area, rect2);

    int smaller_area = rect1.get_area() < rect2.get_area() ? 0 : 1;
    int bigger_area = (smaller_area + 1) % 2;

    // Make the new (smaller) area active
    this->active_area = splitter->areas[smaller_area];
  }

  return splitter;
}

void User_Interface::set_movement_boundaries(Area_Splitter *splitter) {
  int position_min;
  int position_max;
  if (splitter->is_vertical) {
    position_min = splitter->parent_area->left;
    position_max = splitter->parent_area->right;
  } else {
    position_min = splitter->parent_area->bottom;
    position_max = splitter->parent_area->top;
  }

  // Look at all splitters which are under our parent area
  for (int i = 0; i < this->num_splitters; ++i) {
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
    kMargin = Area::kPanelHeight;
  }
  splitter->position_max = position_max - kMargin;
  splitter->position_min = position_min + kMargin;
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
  main_area->reposition_splitter(width_ratio, height_ratio);
}

Update_Result User_Interface::update_and_draw(User_Input *input,
                                              Program_State *state) {
  TIMED_BLOCK();

  Update_Result result = {};
  User_Interface *ui = this;

  if (buffer->was_resized) {
    ui->resize_window(buffer->width, buffer->height);
    buffer->was_resized = false;
  }

  // ----- UI actions (i.e. splitting, moving areas, deleting) ---------------

  for (int i = 0; i < this->num_areas; ++i) {
    Area *area = this->areas[i];
    if (!area->is_visible()) continue;  // ignore wrapper areas

    if (input->button_went_down(IB_mouse_left)) {
      if (area->mouse_over_split_handle(input->mouse)) {
        ui->area_being_split = area;
      } else if (area->mouse_over_delete_button(input->mouse)) {
        ui->area_being_deleted = area;
      }
    }

    // Deleting?
    if (input->button_went_up(IB_mouse_left)) {
      if (ui->area_being_deleted == area &&
          area->mouse_over_delete_button(input->mouse) && i > 0) {
        // Note we're not deleting area 0
        ui->remove_area(area);
        if (ui->active_area == area) {
          ui->active_area = NULL;
        }
      }
    }

    // Make the area active on mousedown, always
    if (area->get_rect().contains(input->mouse) &&
        (input->button_went_down(IB_mouse_left) ||
         input->button_went_down(IB_mouse_middle) ||
         input->button_went_down(IB_mouse_right) || input->scroll != 0)) {
      ui->active_area = area;
    }
  }

  // Select the biggest area if none is active
  if (ui->active_area == NULL) {
    int area_max = 0;
    for (int i = 0; i < this->num_areas; ++i) {
      Area *area = this->areas[i];
      if (!area->is_visible()) continue;  // ignore wrapper areas

      int rect_area = area->get_rect().get_area();
      if (rect_area > area_max) {
        area_max = rect_area;
        ui->active_area = area;
      }
    }
  }
  assert(ui->active_area != NULL);

  // Split areas or move splitters
  if (input->button_is_down(IB_mouse_left)) {
    if (ui->area_being_split != NULL) {
      // See if mouse has moved enough to finish the split
      if (ui->area_being_split->get_rect().contains(input->mouse)) {
        const int kDistance = 25;
        v2i distance = input->mouse_positions[IB_mouse_left] - input->mouse;
        distance.x = abs(distance.x);
        distance.y = abs(distance.y);
        if (distance.x > kDistance || distance.y > kDistance) {
          Area_Splitter *splitter;
          bool is_vertical = distance.x > distance.y;
          splitter =
              ui->split_area(ui->area_being_split, input->mouse, is_vertical);
          ui->splitter_being_moved = splitter;
          ui->set_movement_boundaries(splitter);
          ui->area_being_split = NULL;
        }
      } else {
        // Stop splitting if mouse cursor is outside of the area
        ui->area_being_split = NULL;
      }
    } else {
      // Only look at splitters if areas are not being split
      if (ui->splitter_being_moved != NULL) {
        // Move splitter if we can
        ui->splitter_being_moved->move(input->mouse);
      } else {
        // See if we're about to move a splitter
        for (int i = 0; i < ui->num_splitters; ++i) {
          Area_Splitter *splitter = ui->splitters[i];
          if (splitter->is_mouse_over(input->mouse) &&
              input->button_went_down(IB_mouse_left)) {
            ui->splitter_being_moved = splitter;
            ui->set_movement_boundaries(splitter);
          }
        }
      }
    }
  } else {
    // Mouse button released or not pressed
    ui->area_being_split = NULL;
    ui->area_being_deleted = NULL;
    ui->splitter_being_moved = NULL;
  }

  // ------- Draw area contents -------------------------------------------

  // Potentially will be in separate threads

  // TODO: maybe take this completely away from ui::draw?

  // Clear
  memset(buffer->memory, 0x36, buffer->width * buffer->height * sizeof(u32));

  if (this->z_buffer == NULL) {
    this->z_buffer =
        (r32 *)malloc(buffer->max_width * buffer->max_height * sizeof(r32));
  }
  TIMED_BEGIN(memset);
  memset(this->z_buffer, 0, buffer->width * buffer->height * sizeof(r32));
  TIMED_END(memset);

  // for (int i = 0; i < buffer->width * buffer->height; ++i) {
  //   this->z_buffer[i] = -INFINITY;
  // }

  // Update and draw areas
  for (int i = 0; i < this->num_areas; ++i) {
    Area *area = this->areas[i];
    if (!area->is_visible()) continue;  // ignore wrapper areas

    // Draw editor contents
    switch (area->editor_type) {
      case Area_Editor_Type_3DView: {
        if (!area->editor_3dview.is_drawn) {
          area->editor_3dview.draw(buffer, z_buffer, state, input);
        }
      } break;

      default: { assert(!"Unknown editor type"); } break;
    }

    // Draw panels
    Rect panel_rect = {};
    panel_rect.top = Area::kPanelHeight;
    panel_rect.right = area->get_width();
    draw_rect(area, panel_rect, 0x00686868);

    // Draw type select
    area->type_select.update_and_draw(input);
  }

  // TODO: optimize multiple loops.
  // or maybe use an iterator if we need to keep them?

  // ------ Draw UI elements ----------------------------------------------

  // Draw splitters
  for (int i = 0; i < ui->num_splitters; ++i) {
    Area_Splitter *splitter = ui->splitters[i];
    Area *area = splitter->parent_area;
    int left, top, right, bottom;
    if (splitter->is_vertical) {
      left = splitter->position;
      top = area->top;
      bottom = area->bottom;
      draw_line(buffer, V2i(left - 1, top), V2i(left - 1, bottom), 0x00323232);
      draw_line(buffer, V2i(left, top), V2i(left, bottom), 0x00000000);
      draw_line(buffer, V2i(left + 1, top), V2i(left + 1, bottom), 0x00505050);
    } else {
      top = splitter->position;
      left = area->left;
      right = area->right;
      draw_line(buffer, V2i(left, top - 1), V2i(right, top - 1), 0x00000000);
      draw_line(buffer, V2i(left, top), V2i(right, top), 0x00505050);
    }
  }

  for (int i = 0; i < ui->num_areas; ++i) {
    Area *area = ui->areas[i];
    if (!area->is_visible()) continue;

    // Draw split handles
    draw_rect(area, area->get_split_handle(0), 0x00777777);
    draw_rect(area, area->get_split_handle(1), 0x00777777);

    // Draw delete buttons
    if (i > 0) {
      // Can't delete area 0
      draw_rect(area, area->get_delete_button(), 0x00772222);
    }
  }

  // ------- Cursors ---------------------------------------------

  // Splitter resize cursor
  for (int i = 0; i < ui->num_splitters; ++i) {
    Area_Splitter *splitter = ui->splitters[i];
    if (splitter->get_rect().contains(input->mouse)) {
      result.cursor = splitter->is_vertical ? Cursor_Type_Resize_Horiz
                                            : Cursor_Type_Resize_Vert;
    }
  }

  // Area split cursor
  for (int i = 0; i < ui->num_areas; ++i) {
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
