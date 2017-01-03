
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
      Model model = state->models[0];  // cube
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

    // For AABB - note we assume the position is within the model
    v3 min = model->position;
    v3 max = model->position;

    // #pragma omp parallel for
    for (int f = 0; f < sb_count(model->faces); ++f) {
      Face face = model->faces[f];
      v3 scene_verts[3];
      v3 screen_verts[3];
      v2 texture_verts[3];
      v3 vns[3];

      for (int i = 0; i < 3; ++i) {
        scene_verts[i] = ModelTransform * model->vertices[face.v_ids[i]];

        // Find AABB. This is probably not a good idea since it
        // won't work if the next frame the model drastically changes its
        // position, but we'll see
        for (int j = 0; j < 3; ++j) {
          r32 value = scene_verts[i].E[j];
          if (value < min.E[j]) {
            min.E[j] = value;
          } else if (max.E[j] < value) {
            max.E[j] = value;
          }
        }

        // texture_verts[i] = model->vts[face.vt_ids[i]];
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

    // Update bounding box
    model->aabb.min = min;
    model->aabb.max = max;

    if (model->debug) {
      // Draw the direction vector
      draw_line(buffer, WorldTransform * model->position,
                WorldTransform * (model->position + model->direction * 0.3f),
                0x00FF0000, z_buffer);

      // Draw AABBoxes
      v3 verts[] = {
          {min.x, min.y, min.z},
          {min.x, min.y, max.z},
          {max.x, min.y, max.z},
          {max.x, min.y, min.z},
          {max.x, max.y, min.z},
          {min.x, max.y, min.z},
          {min.x, max.y, max.z},
          {max.x, max.y, max.z},
      };
      int lines[] = {0, 1, 1, 2, 2, 3, 3, 0, 5, 6, 6, 7,
                     7, 4, 4, 5, 0, 5, 3, 4, 1, 6, 2, 7};
      // assert(COUNT_OF(lines) % 2 == 0);
      for (size_t i = 0; i < COUNT_OF(lines); i += 2) {
        draw_line(buffer, WorldTransform * verts[lines[i]],
                  WorldTransform * verts[lines[i + 1]], 0x00FFAA40, z_buffer);
      }
      // draw_line(buffer, V3(min.x, 0.0f, min.z), V3(min.x, 0.0f, max.z),
      // 0x00FFAA40, z_buffer);
      // draw_line(buffer, V3(max.x, 0.0f, max.z), V3(max.x, 0.0f, min.z),
      // 0x00FFAA40, z_buffer);
      // draw_line(buffer, V3(max.x, 0.0f, max.z), V3(min.x, 0.0f, max.z),
      // 0x00FFAA40, z_buffer);

      // draw_line(buffer, min, V3(min.x, min.y, max.z), 0x00000099, z_buffer);
      // draw_line(buffer, min, V3(min.x, min.y, max.z), 0x00000099, z_buffer);
      // draw_line(buffer, min, V3(max.x, min.y, min.z), 0x00990000, z_buffer);
      // draw_line(buffer, min, V3(min.x, max.y, min.z), 0x00009900, z_buffer);
      // draw_line(buffer, max, V3(max.x, max.y, min.z), 0x00FFAA40, z_buffer);
      // draw_line(buffer, max, V3(max.x, min.y, max.z), 0x00FFAA40, z_buffer);
      // draw_line(buffer, max, V3(min.x, max.y, max.z), 0x00FFAA40, z_buffer);
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