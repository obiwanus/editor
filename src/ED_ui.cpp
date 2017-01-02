
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

inline void draw_pixel(Pixel_Buffer *buffer, v2i point, u32 color,
                       bool top_left = false) {
  int x = point.x;
  int y = point.y;

  if (!top_left) {
    y = buffer->height - y;  // Origin in bottom-left
  }
  u32 *pixel = (u32 *)buffer->memory + x + y * buffer->width;
  *pixel = color;
}

void draw_line(Pixel_Buffer *buffer, v2i A, v2i B, u32 color, int width = 1,
               bool top_left = false) {
  bool swapped = false;
  if (abs(B.x - A.x) < abs(B.y - A.y)) {
    swap(A.x, A.y);
    swap(B.x, B.y);
    swapped = true;
  }
  if (B.x < A.x) {
    swap(A, B);
  }
  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = (dy >= 0) ? 1 : -1;
  int error = sign * dy - dx;
  int error_step = sign * dy;
  int y = A.y;
  for (int x = A.x; x <= B.x; ++x) {
    for (int i = 0; i < width; ++i) {
      int X = x, Y = y + i;
      if (swapped) {
        X = y + i;
        Y = x;
      }
      if (X >= 0 && X < buffer->width && Y >= 0 && Y < buffer->height) {
        draw_pixel(buffer, V2i(X, Y), color, top_left);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_ui_line(Pixel_Buffer *buffer, v2i A, v2i B, u32 color) {
  // Draw line with the top left corner as the origin
  draw_line(buffer, A, B, color, 1, true);
}

void draw_line(Pixel_Buffer *buffer, v3 Af, v3 Bf, u32 color, r32 *z_buffer) {
  // Super unoptimized, as everything related to drawing so far
  bool swapped = false;
  if (abs(Bf.x - Af.x) < abs(Bf.y - Af.y)) {
    swap(Af.x, Af.y);
    swap(Bf.x, Bf.y);
    swapped = true;
  }
  if (Bf.x < Af.x) {
    swap(Af, Bf);
  }
  v3i A = V3i(Af);
  v3i B = V3i(Bf);
  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = (dy >= 0) ? 1 : -1;
  int error = sign * dy - dx;
  int error_step = sign * dy;
  int y = A.y;

  for (int x = A.x; x <= B.x; ++x) {
    int X = x, Y = y;
    if (swapped) swap(X, Y);  // unswap for drawing
    if (X >= 0 && X < buffer->width && Y >= 0 && Y < buffer->height) {
      r32 t = (r32)(x - A.x) / dx;
      r32 z = (1.0f - t) * Af.z + t * Bf.z;
      int index = buffer->width * Y + X;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(buffer, V2i(X, Y), color, false);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void triangle_wireframe(Pixel_Buffer *buffer, v3 verts[], u32 color) {
  v2i vert0 = V2i(verts[0]);
  v2i vert1 = V2i(verts[1]);
  v2i vert2 = V2i(verts[2]);
  draw_line(buffer, vert0, vert1, color);
  draw_line(buffer, vert0, vert2, color);
  draw_line(buffer, vert1, vert2, color);
}

void triangle_filled(Pixel_Buffer *buffer, v3 verts[], u32 color,
                     r32 *z_buffer) {
  // clang-format off
  v2i t0 = V2i(verts[0]); r32 z0 = verts[0].z;
  v2i t1 = V2i(verts[1]); r32 z1 = verts[1].z;
  v2i t2 = V2i(verts[2]); r32 z2 = verts[2].z;

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  if (t0.y > t1.y) { swap(t0, t1); swap(z0, z1); }
  if (t0.y > t2.y) { swap(t0, t2); swap(z0, z2); }
  if (t1.y > t2.y) { swap(t1, t2); swap(z1, z2); }

  int total_height = t2.y - t0.y;
  for (int y = t0.y; y <= t2.y; y++) {
    if (y < 0 || y >= buffer->height) continue;
    bool second_half = y > t1.y || t1.y == t0.y;
    int segment_height = second_half ? (t2.y - t1.y) : (t1.y - t0.y);
    r32 dy_total = (r32)(y - t0.y) / total_height;
    r32 dy_segment =
        (r32)(second_half ? (y - t1.y) : (y - t0.y)) / segment_height;
    v2i A = lerp(t0, t2, dy_total);
    r32 A_z = lerp(z0, z2, dy_total);
    v2i B;
    r32 B_z;
    if (!second_half) {
      B = lerp(t0, t1, dy_segment);
      B_z = lerp(z0, z1, dy_segment);
    } else {
      B = lerp(t1, t2, dy_segment);
      B_z = lerp(z1, z2, dy_segment);
    }
    if (A.x > B.x) { swap(A, B); swap(A_z, B_z); };
    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(buffer, V2i(x, y), color, false);
      }
    }
  }
  // clang-format on
}

void triangle_shaded(Pixel_Buffer *buffer, v3 verts[], v3 vns[], r32 *z_buffer,
                     v3 light_dir, bool outline = false) {
  // clang-format off
  v2i t0 = V2i(verts[0]); r32 z0 = verts[0].z;
  v2i t1 = V2i(verts[1]); r32 z1 = verts[1].z;
  v2i t2 = V2i(verts[2]); r32 z2 = verts[2].z;

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  // Calculate intensity at vertices for Gouraud shading
  light_dir = light_dir.normalized();
  r32 in[3];
  for (int i = 0; i < 3; ++i) {
    in[i] = -vns[i].normalized() * light_dir;
  }

  if (t0.y > t1.y) { swap(t0, t1); swap(z0, z1); swap(in[0], in[1]); }
  if (t0.y > t2.y) { swap(t0, t2); swap(z0, z2); swap(in[0], in[2]); }
  if (t1.y > t2.y) { swap(t1, t2); swap(z1, z2); swap(in[1], in[2]); }

  int total_height = t2.y - t0.y;
  for (int y = t0.y; y <= t2.y; y++) {
    if (y < 0 || y >= buffer->height) continue;
    bool second_half = y > t1.y || t1.y == t0.y;
    int segment_height = second_half ? (t2.y - t1.y) : (t1.y - t0.y);
    r32 dy_total = (r32)(y - t0.y) / total_height;
    r32 dy_segment =
        (r32)(second_half ? (y - t1.y) : (y - t0.y)) / segment_height;
    v2i A = lerp(t0, t2, dy_total);
    r32 A_z = lerp(z0, z2, dy_total);
    r32 A_in = lerp(in[0], in[2], dy_total);
    v2i B;
    r32 B_z;
    r32 B_in;
    if (!second_half) {
      B = lerp(t0, t1, dy_segment);
      B_z = lerp(z0, z1, dy_segment);
      B_in = lerp(in[0], in[1], dy_segment);
    } else {
      B = lerp(t1, t2, dy_segment);
      B_z = lerp(z1, z2, dy_segment);
      B_in = lerp(in[1], in[2], dy_segment);
    }
    if (A.x > B.x) { swap(A, B); swap(A_z, B_z); swap(A_in, B_in); };
    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        r32 intensity = lerp(A_in, B_in, t);
        if (intensity < 0) intensity = 0;
        intensity = lerp(0.2f, 1.0f, intensity);
        const r32 grey = 0.7f;
        u32 color = get_rgb_u32(V3(grey, grey, grey) * intensity);
        if (outline && (x == A.x || x == B.x || y == t0.y || y == t2.y)) {
          color = 0x00FFAA40;
        }
        draw_pixel(buffer, V2i(x, y), color, false);
      }
    }
  }
  // clang-format on
}

void triangle_textured(Pixel_Buffer *buffer, v3i verts[], v2 vts[], v3 vns[],
                       Image texture, r32 *z_buffer) {
  v3i t0 = verts[0];
  v3i t1 = verts[1];
  v3i t2 = verts[2];

  v2 tex0 = vts[0];
  v2 tex1 = vts[1];
  v2 tex2 = vts[2];

  v3 n0 = vns[0];
  v3 n1 = vns[1];
  v3 n2 = vns[2];

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  if (t0.y > t1.y) {
    swap(t0, t1);
    swap(tex0, tex1);
    swap(n0, n1);
  }
  if (t0.y > t2.y) {
    swap(t0, t2);
    swap(tex0, tex2);
    swap(n0, n2);
  }
  if (t1.y > t2.y) {
    swap(t1, t2);
    swap(tex1, tex2);
    swap(n1, n2);
  }

  int total_height = t2.y - t0.y;
  for (int y = t0.y; y <= t2.y; y++) {
    if (y < 0 || y >= buffer->height) continue;
    bool second_half = y > t1.y || t1.y == t0.y;
    int segment_height = second_half ? (t2.y - t1.y + 1) : (t1.y - t0.y + 1);
    r32 dy_total = (r32)(y - t0.y) / total_height;
    r32 dy_segment =
        (r32)(second_half ? (y - t1.y) : (y - t0.y)) / segment_height;
    v3i A = t0 + V3i(dy_total * V3(t2 - t0));  // TODO: try lerp
    v3i B;
    v2 Atex = tex0 + dy_total * (tex2 - tex0);
    v2 Btex;
    v3 An = n0 + dy_total * (n2 - n0);
    v3 Bn;
    if (!second_half) {
      B = t0 + V3i(dy_segment * V3(t1 - t0));
      Btex = tex0 + dy_segment * (tex1 - tex0);
      Bn = n0 + dy_segment * (n1 - n0);
    } else {
      B = t1 + V3i(dy_segment * V3(t2 - t1));
      Btex = tex1 + dy_segment * (tex2 - tex1);
      Bn = n1 + dy_segment * (n2 - n1);
    }
    if (A.x > B.x) {
      swap(A, B);
      swap(Atex, Btex);
      swap(An, Bn);
    };
    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = (r32)lerp(A.z, B.z, t);
      int index = buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        v3 n = lerp(An, Bn, t).normalized();
        r32 intensity = n * V3(0, 0, 1);
        if (intensity > 0) {
          // // Get color from texture
          v2 texel = lerp(Atex, Btex, t);
          v2i tex_coords =
              V2i(texture.width * texel.u, texture.height * texel.v);
          u32 color = texture.color(tex_coords.x,
                                    (texture.height - tex_coords.y), intensity);
          draw_pixel(buffer, V2i(x, y), color);
        }
      }
    }
  }
}

// void draw_ui_line(Pixel_Buffer *buffer, v2 A, v2 B, u32 color) {
//   v2i a = {(int)A.x, (int)A.y};
//   v2i b = {(int)B.x, (int)B.y};
//   draw_ui_line(buffer, a, b, color);
// }

void draw_rect(Pixel_Buffer *buffer, Rect rect, u32 color,
               bool top_left = true) {
  if (rect.left < 0) rect.left = 0;
  if (rect.top < 0) rect.top = 0;
  if (rect.right > buffer->width) rect.right = buffer->width;
  if (rect.bottom > buffer->height) rect.bottom = buffer->height;

  for (int x = rect.left; x < rect.right; x++) {
    for (int y = rect.top; y < rect.bottom; y++) {
      // Don't care about performance (yet)
      draw_pixel(buffer, V2i(x, y), color, top_left);
    }
  }
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

bool Area::mouse_over_split_handle(v2i mouse) {
  return this->get_split_handle(0).contains(mouse) ||
         this->get_split_handle(1).contains(mouse);
}

bool Area::is_visible() { return this->splitter == NULL; }

void Area::destroy() {
  if (this->buffer.memory != NULL) {
    free(this->buffer.memory);
  this->buffer.memory = NULL;
  }
  if (this->z_buffer != NULL) {
    free(this->z_buffer);
    this->z_buffer = NULL;
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

Rect UI_Select::get_rect() {
  // Strange that I was aiming at generalisation but made
  // this function extremely specific to type selects
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

void UI_Select::update_and_draw(User_Input *input) {
  UI_Select *select = this;
  assert(select->parent_area != NULL);

  Rect area_rect = select->parent_area->get_rect();
  Rect select_rect = select->get_rect();
  Pixel_Buffer *buffer = &select->parent_area->buffer;

  // Draw unhighlighted no matter what
  const u32 colors[10] = {0x00000000, 0x00123123};
  draw_rect(buffer, select_rect,
            colors[select->option_selected]);

  // If mouse cursor is not in the area, don't interact with select
  if (!area_rect.contains(input->mouse)) {
    select->open = false;
    return;
  }

  Rect all_options_rect;
  int kMargin = 1;
  all_options_rect.left = select_rect.left - kMargin;
  all_options_rect.right = select_rect.right + 30 + kMargin;
  all_options_rect.bottom = select_rect.bottom + kMargin;
  all_options_rect.top = select_rect.top -
                         select->option_count * (select->option_height + 1) -
                         kMargin + 1;

  // Update
  bool mouse_over_select =
      select_rect.contains(area_rect.projected(input->mouse));
  bool mouse_over_options =
      all_options_rect.contains(area_rect.projected(input->mouse));

  if (mouse_over_select && input->button_went_down(IB_mouse_left)) {
    select->open = true;
  }

  if (select->open && !mouse_over_options) {
    select->open = false;
  }

  if (mouse_over_select || select->open) {
    // Draw highlighted
    draw_rect(buffer, select_rect, 0x00222222);
  }

  if (select->open) {
    Rect options_border = all_options_rect;
    options_border.bottom = select_rect.top + kMargin;

    // Draw all options rect first
    draw_rect(buffer, options_border, 0x00686868);

    int bottom = select_rect.top;
    for (int opt = 0; opt < select->option_count; opt++) {
      Rect option;
      option.bottom = bottom;
      option.left = select_rect.left;
      option.right = select_rect.right + 30;
      option.top = option.bottom - select->option_height;
      bool mouse_over_option =
          mouse_over_options &&
          option.contains(area_rect.projected(input->mouse));
      u32 color = colors[opt];
      if (mouse_over_option) {
        color += 0x00121212;  // highlight
      }
      draw_rect(buffer, option, color);
      bottom -= select->option_height + 1;

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

  // Init type selector
  {
    UI_Select *select = &area->type_select;
    *select = {};
    select->align_bottom = true;
    select->x = 20;
    select->y = 3;
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

  // Create draw buffer
  {
    area->buffer.allocate();
    area->buffer.width = area->get_width();
    area->buffer.height = area->get_height();

    area->z_buffer = NULL;
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
  {
    parent_area->editor_3dview.camera = sister_area->editor_3dview.camera;
    parent_area->z_buffer = sister_area->z_buffer;
  }

  // Remove z-buffer if any
  if (area->z_buffer != NULL) {
    free(area->z_buffer);
    area->z_buffer = NULL;
  }

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
    assert(0 <= area_id && area_id < this->num_areas);
    assert(0 <= sister_area_id && sister_area_id < this->num_areas);
    int edge = this->num_areas - 1;
    if (area_id < edge) {
      this->areas[area_id]->destroy();
      this->areas[area_id] = this->areas[edge];
      this->areas[edge] = {};
      if (sister_area_id == edge) {
        // Make sure we overwrite it later
        sister_area_id = area_id;
      }
    }
    edge--;
    if (sister_area_id < edge) {
      this->areas[sister_area_id]->destroy();
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

    // Use parent z-buffer in the bigger area
    splitter->areas[bigger_area]->z_buffer = area->z_buffer;
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
    position_min = splitter->parent_area->top;
    position_max = splitter->parent_area->bottom;
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

Update_Result User_Interface::update_and_draw(Pixel_Buffer *buffer,
                                              User_Input *input,
                                              Program_State *state) {
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
      } else if (area->get_delete_button().contains(input->mouse)) {
        ui->area_being_deleted = area;
      }
    }

    // Deleting?
    if (input->button_went_up(IB_mouse_left)) {
      if (ui->area_being_deleted == area &&
          area->get_delete_button().contains(input->mouse) && i > 0) {
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

  // Update and draw areas
  for (int i = 0; i < this->num_areas; ++i) {
    Area *area = this->areas[i];
    if (!area->is_visible()) continue;  // ignore wrapper areas
    area->buffer.width = area->get_width();
    area->buffer.height = area->get_height();

    // Draw editor contents
    switch (area->editor_type) {
      case Area_Editor_Type_3DView: {
        if (!area->editor_3dview.is_drawn) {
          area->editor_3dview.draw(state, input);
        }
      } break;

      case Area_Editor_Type_Raytrace: {
        area->editor_raytrace.draw(ui);
      } break;

      default: { assert(!"Unknown editor type"); } break;
    }

    // Draw panels
    Rect panel_rect = area->buffer.get_rect();
    panel_rect.top = panel_rect.bottom - Area::kPanelHeight;
    draw_rect(&area->buffer, panel_rect, 0x00686868);

    // Draw type select
    area->type_select.update_and_draw(input);

    // Any actions within area
    Rect area_rect = area->get_rect();
    if (area_rect.contains(input->mouse)) {
    }
  }

  // Copy area buffers into the main buffer
  for (int i = 0; i < ui->num_areas; ++i) {
    Area *area = ui->areas[i];
    if (!area->is_visible()) continue;
    Rect client_rect = area->get_client_rect();
    Pixel_Buffer *src_buffer = &area->buffer;
    for (int y = 0; y < src_buffer->height; y++) {
      for (int x = 0; x < src_buffer->width; x++) {
        u32 *pixel_src = (u32 *)src_buffer->memory + x + y * src_buffer->width;
        u32 *pixel_dest = (u32 *)buffer->memory + (client_rect.left + x) +
                          (client_rect.top + y) * buffer->width;
        *pixel_dest = *pixel_src;
      }
    }
  }

  // TODO: optimize multiple loops.
  // or maybe use an iterator if we need to keep them?

  // ------ Draw UI elements ----------------------------------------------

  // Draw splitters
  for (int i = 0; i < ui->num_splitters; ++i) {
    Area_Splitter *splitter = ui->splitters[i];
    // draw_rect(buffer, splitter->get_rect(), {1,1,1});
    Area *area = splitter->parent_area;
    int left, top, right, bottom;
    if (splitter->is_vertical) {
      left = splitter->position;
      top = area->top;
      bottom = area->bottom;
      draw_ui_line(buffer, V2i(left - 1, top), V2i(left - 1, bottom),
                   0x00323232);
      draw_ui_line(buffer, V2i(left, top), V2i(left, bottom), 0x00000000);
      draw_ui_line(buffer, V2i(left + 1, top), V2i(left + 1, bottom),
                   0x00505050);
    } else {
      top = splitter->position;
      left = area->left;
      right = area->right;
      draw_ui_line(buffer, V2i(left, top - 1), V2i(right, top - 1), 0x00000000);
      draw_ui_line(buffer, V2i(left, top), V2i(right, top), 0x00505050);
    }
  }

  for (int i = 0; i < ui->num_areas; ++i) {
    Area *area = ui->areas[i];
    if (!area->is_visible()) continue;

    // Draw split handles
    draw_rect(buffer, area->get_split_handle(0), 0x00777777);
    draw_rect(buffer, area->get_split_handle(1), 0x00777777);

    // Draw delete buttons
    if (i > 0) {
      // Can't delete area 0
      draw_rect(buffer, area->get_delete_button(), 0x00772222);
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

void Editor_3DView::draw(Program_State *state, User_Input *input) {
  User_Interface *ui = state->UI;
  Pixel_Buffer *buffer = &this->area->buffer;
  bool active = ui->active_area == this->area;

  {
    u8 bgcolor = active ? 0x3A : 0x36;
    memset(buffer->memory, bgcolor,
           buffer->width * buffer->height * sizeof(u32));
  }
  this->camera.adjust_frustum(buffer->width, buffer->height);

  if (active) {
    v2i mouse_position = this->area->get_rect().projected_to_area(input->mouse);
    Ray ray = this->camera.get_ray_through_pixel(mouse_position);

    if (input->button_went_down(IB_mouse_left)) {
      // Set cursor position to the point of intersection between the ray
      // and the plane passing through the camera pivot and orthogonal
      // to camera's direction
      r32 t = (this->camera.pivot - ray.origin) * this->camera.direction /
              (ray.direction * this->camera.direction);
      ui->cursor = ray.point_at(t);
    } else if (input->button_went_down(IB_mouse_right)) {
      // Attempt to select a model
      // TODO: add bounding box check
      // (I don't like this loop)
      for (int m = 0; m < sb_count(state->models); ++m) {
        Model *model = state->models + m;
        if (!model->display) continue;
        if (model == state->selected_model) continue;

        // Put model in the scene
        m4x4 ModelTransform =
            Matrix::frame_to_canonical(model->get_basis(), model->position) *
            Matrix::S(model->scale);

        for (int f = 0; f < sb_count(model->faces); ++f) {
          Face face = model->faces[f];
          Triangle triangle;
          for (int i = 0; i < 3; ++i) {
            triangle.vertices[i] =
                ModelTransform * model->vertices[face.v_ids[i]];
          }
          if (triangle.hit_by(ray) > 0) {
            state->selected_model = model;
            break;
          }
        }
      }
    }
    if (input->key_went_down('A')) {
      Model model = state->models[1];  // cube
      model.position = ui->cursor;
      model.direction = (this->camera.position - model.position).normalized();
      sb_push(state->models, model);
    }
    if (input->key_went_down('5')) {
      this->camera.ortho_projection = !this->camera.ortho_projection;
    }
    if (input->button_went_down(IB_mouse_middle)) {
      // Remember position
      this->camera.old_position = this->camera.position;
      this->camera.old_up = this->camera.up;
      this->camera.old_basis = this->camera.get_basis();
      this->camera.old_pivot = this->camera.pivot;
      if (input->button_is_down(IB_shift)) {
        this->mode = Editor_3DView_Mode_Pivot_Move;
      } else {
        this->mode = Editor_3DView_Mode_Camera_Rotate;
      }
    }
    if (input->button_is_down(IB_mouse_middle)) {
      v2 delta = V2(input->mouse_positions[IB_mouse_middle] - input->mouse);
      if (this->mode == Editor_3DView_Mode_Camera_Rotate) {
        this->camera.pivot = this->camera.old_pivot;
        // Rotate camera around pivot
        const int kSensitivity = 500;
        v2 angles = (M_PI / kSensitivity) * delta;
        m4x4 CameraRotate = this->camera.rotation_matrix(angles);
        this->camera.position = CameraRotate * this->camera.old_position;
        this->camera.up = V3(CameraRotate * V4_v(this->camera.old_up));
      } else if (this->mode == Editor_3DView_Mode_Pivot_Move) {
        // Move the pivot
        r32 sensitivity = 0.001f * this->camera.distance_to_pivot();
        this->camera.up = this->camera.old_up;
        basis3 basis = this->camera.old_basis;
        v3 right = basis.u;
        v3 up = basis.v;
        v3 move_vector = (right * delta.x - up * delta.y) * sensitivity;
        this->camera.pivot = this->camera.old_pivot + move_vector;
        this->camera.position = this->camera.old_position + move_vector;
      }
      this->camera.look_at(this->camera.pivot);
    } else {
      this->mode = Editor_3DView_Mode_Normal;
    }
    if (input->scroll && !input->button_is_down(IB_mouse_middle)) {
      // Move camera on scroll
      this->camera.position += -this->camera.direction *
                               this->camera.distance_to_pivot() *
                               (input->scroll / 10.0f);
    }
  }

  m4x4 CameraSpaceTransform = camera.transform_to_entity_space();

  m4x4 ProjectionMatrix = camera.projection_matrix();

  m4x4 ViewportTransform =
      Matrix::viewport(0, 0, buffer->width, buffer->height);

  m4x4 WorldTransform =
      ViewportTransform * ProjectionMatrix * CameraSpaceTransform;

  if (this->area->z_buffer == NULL) {
    this->area->z_buffer =
        (r32 *)malloc(buffer->max_width * buffer->max_height * sizeof(r32));
  }
  r32 *z_buffer = this->area->z_buffer;
  for (int i = 0; i < buffer->width * buffer->height; ++i) {
    z_buffer[i] = -INFINITY;
  }

  for (int m = 0; m < sb_count(state->models); ++m) {
    Model *model = state->models + m;
    if (!model->display) continue;

    // Put model in the scene
    m4x4 ModelTransform =
        Matrix::frame_to_canonical(model->get_basis(), model->position) *
        Matrix::S(model->scale);

    for (int f = 0; f < sb_count(model->faces); ++f) {
      Face face = model->faces[f];
      v3 scene_verts[3];
      v3 screen_verts[3];
      v2 texture_verts[3];
      v3 vns[3];

      for (int i = 0; i < 3; ++i) {
        scene_verts[i] = ModelTransform * model->vertices[face.v_ids[i]];
        texture_verts[i] = model->vts[face.vt_ids[i]];
        screen_verts[i] = WorldTransform * scene_verts[i];
        vns[i] = model->vns[face.vn_ids[i]];
        vns[i] = V3(ModelTransform * V4_v(vns[i])).normalized();
      }

      v3 light_dir = -this->camera.direction;

      bool outline = (model == state->selected_model);
      triangle_shaded(buffer, screen_verts, vns, z_buffer, light_dir, outline);

      // triangle_wireframe(buffer, screen_verts, 0x00FFAA40);

      // {
      //   // Draw single color grey facets
      //   v3 vert1 = scene_verts[0];
      //   v3 vert2 = scene_verts[1];
      //   v3 vert3 = scene_verts[2];
      //   v3 n = ((vert3 - vert1).cross(vert2 - vert1)).normalized();
      //   r32 intensity = n * light_dir;
      //   if (intensity < 0) { intensity = 0; }
      //   intensity = lerp(0.2f, 1.0f, intensity);
      //   const r32 grey = 0.7f;
      //   u32 color = get_rgb_u32(V3(grey, grey, grey) * intensity);
      //   triangle_filled(buffer, screen_verts, color, z_buffer);
      // }

      // // Debug draw normals
      // for (int j = 0; j < 3; ++j) {
      //   scene_verts[j] = model->vertices[face.v_ids[j]];
      //   vns[j] = model->vns[face.vn_ids[j]];
      //   v3 normal_end = WorldTransform * ModelTransform *
      //                   (scene_verts[j] + (vns[j] * 0.1f));
      //   draw_line(buffer, screen_verts[j], normal_end, 0x00FF0000, z_buffer);
      // }
    }

    if (model->debug) {
      draw_line(buffer, WorldTransform * model->position,
                WorldTransform * (model->position + model->direction),
                0x00FF0000, z_buffer);
    }
  }

  // Draw grid
  const u32 kXColor = 0x00990000;
  const u32 kYColor = 0x00009900;
  const u32 kZColor = 0x00000099;
  {
    const int kLineCount = 21;
    const u32 kGridColor = 0x00555555;
    for (int i = 0; i < kLineCount; ++i) {
      u32 color = kGridColor;
      v3 vert1, vert2;
      r32 step = -1.0f + i * 2.0f / (kLineCount - 1);
      // Along the x axis
      {
        vert1 = WorldTransform * V3(-1.0f, 0.0f, step);
        vert2 = WorldTransform * V3(1.0f, 0.0f, step);
        if (i == kLineCount / 2) {
          color = kXColor;
        }
        draw_line(buffer, vert1, vert2, color, z_buffer);
      }
      // Along the z axis
      {
        vert1 = WorldTransform * V3(step, 0.0f, -1.0);
        vert2 = WorldTransform * V3(step, 0.0f, 1.0);
        if (i == kLineCount / 2) {
          color = kZColor;
        }
        draw_line(buffer, vert1, vert2, color, z_buffer);
      }
    }
  }

  // Draw cursor
  {
    v2i cursor = V2i(WorldTransform * ui->cursor);
    Rect cursor_rect = {cursor.x - 3, cursor.y - 3, cursor.x + 3, cursor.y + 3};
    draw_rect(buffer, cursor_rect, 0x00FF0000, false);
  }

  // Draw the axis in the corner
  {
    m4x4 IconViewport = Matrix::viewport(5, 30, 60, 60);
    v3 x = V3(CameraSpaceTransform * V4(1, 0, 0, 0));
    v3 y = V3(CameraSpaceTransform * V4(0, 1, 0, 0));
    v3 z = V3(CameraSpaceTransform * V4(0, 0, 1, 0));

    v2i origin = V2i(IconViewport * V3(0, 0, 0));

    draw_line(buffer, origin, V2i(IconViewport * x), kXColor, 2);
    draw_line(buffer, origin, V2i(IconViewport * z), kZColor, 2);
    draw_line(buffer, origin, V2i(IconViewport * y), kYColor, 2);
  }

  // test dragging
  // if (ui->active_area == this->area) {
  //   Rect area_rect = this->area->get_rect();
  //   const u32 colors[] = {0x0000FF00, 0x00FF0000, 0x000000FF};
  //   v2i to = area_rect.projected_to_area(input->mouse);
  //   for (int i = 0; i < 3; ++i) {
  //     if (input->button_is_down((Input_Button)i)) {
  //       v2i from = area_rect.projected_to_area(input->mouse_positions[i]);
  //       draw_line(buffer, from, to, colors[i], 4);
  //     }
  //   }
  // }
}

void Editor_Raytrace::draw(User_Interface *ui) {
  // TODO: ray tracer should probably be member and not
  // a function parameter

  // Debug draw texture image
  // u32 *pitch = model->texture.width
  // Pixel_Buffer *buffer = this->area->buffer;

  // // TMP - buggy with bytes per pixel = 3
  // u32 *src = model.texture.data;
  // for (int y = 0; y < model.texture.height; ++y) {
  //   if (y >= buffer->height) break;
  //   for (int x = 0; x < model.texture.width; ++x) {
  //     if (x >= buffer->width) continue;
  //     u32 *pixel = (u32 *)buffer->memory + x + y * buffer->width;
  //     u32 value = *(src + x);
  //     *pixel = value;
  //   }
  //   src += model.texture.width;
  // }

  // if (rt == NULL) {
  //   // draw_rect(this->area->buffer, this->area->buffer.get_rect(),
  //   //           0x00123123);
  //   return;
  // }

  // RayCamera *camera = &rt->camera;
  // LightSource *lights = rt->lights;
  // RayObject **ray_objects = rt->ray_objects;

  // Rect client_rect = this->area->get_client_rect();

  // v2i pixel_count = {client_rect.right - client_rect.left,
  //                    client_rect.bottom - client_rect.top};

  // // TODO: this is wrong but tmp.
  // // Ideally a raytrace view should have its own camera,
  // // but this is probably not going to happen anyway
  // camera->left = -pixel_count.x / 2;
  // camera->right = pixel_count.x / 2;
  // camera->top = pixel_count.y / 2;
  // camera->bottom = -pixel_count.y / 2;

  // for (int x = 0; x < pixel_count.x; x++) {
  //   for (int y = 0; y < pixel_count.y; y++) {
  //     RTRay ray = camera->get_ray_through_pixel(x, y, pixel_count);

  //     const v3 ambient_color = {0.2f, 0.2f, 0.2f};
  //     const r32 ambient_light_intensity = 0.3f;

  //     v3 color = ambient_color * ambient_light_intensity;

  //     color += ray.get_color(rt, 0, rt->kMaxRecursion);

  //     // Crop
  //     for (int i = 0; i < 3; ++i) {
  //       if (color.E[i] > 1) {
  //         color.E[i] = 1;
  //       }
  //       if (color.E[i] < 0) {
  //         color.E[i] = 0;
  //       }
  //     }

  //     draw_pixel(this->area->buffer, V2i(x, y), get_rgb_u32(color));
  //   }
  // }
}
