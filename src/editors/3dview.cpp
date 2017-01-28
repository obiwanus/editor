
void Editor_3DView::draw(Pixel_Buffer *buffer, r32 *z_buffer,
                         Program_State *state, User_Input *input) {
#if 0
  TIMED_BLOCK();
  // v3 screen_verts[3] = {V3(200.5f, 199.23f, 0.0f), V3(800.3f, 300.2f, 0.0f),
  //                       V3(500.0f, 599.234f, 0.0f)};
  v3 light_dir = V3(1, 1, -1);
  v3 screen_verts[3] = {V3(300, 300, 100), V3(710, 500, 150),
                        V3(505, 709, 130)};
  v3 vns[3] = {V3(0, 0, 1), V3(0, 0, 1), V3(1, 0, 1)};


  v3 screen_verts2[3] = {V3(300, 709, 100), V3(710, 400, 150),
                         V3(505, 300, 130)};

  v3 screen_verts3[3] = {V3(900, 109, 20), V3(710, 400, 150),
                         V3(505, 300, 130)};

  for (int i = 0; i < 3; ++i) {
    screen_verts[i] *= 0.2f;
    screen_verts2[i] *= 0.2f;
    screen_verts3[i] *= 0.2f;
  }

  triangle_rasterize_simd(area, screen_verts, vns, z_buffer, light_dir, false);

  triangle_rasterize_simd(area, screen_verts2, vns, z_buffer, light_dir, false);
  // triangle_rasterize_simd(area, screen_verts3, vns, z_buffer, light_dir, false);

#else
  TIMED_BLOCK();

  User_Interface *ui = state->UI;
  bool active = ui->active_area == this->area;

  int area_width = this->area->get_width();
  int area_height = this->area->get_height();

  this->camera.adjust_frustum(area_width, area_height);

  if (active) {
    v2i mouse_position = this->area->get_rect().projected(input->mouse);
    Ray ray = this->camera.get_ray_through_pixel(mouse_position);

    if (input->button_went_down(IB_mouse_left)) {
      // Set cursor position to the point of intersection between the ray
      // and the plane passing through the camera pivot and orthogonal
      // to camera's direction
      r32 t = (this->camera.pivot - ray.origin) * this->camera.direction /
              (ray.direction * this->camera.direction);
      ui->cursor = ray.get_point_at(t);
    } else if (input->button_went_down(IB_mouse_right)) {
      // Attempt to select a model
      // TODO: add bounding box check
      // (I don't like this loop)
      for (int m = 0; m < sb_count(state->models); ++m) {
        Model *model = state->models + m;
        if (!model->display) continue;
        if (model == state->selected_model) continue;

        // Put model in the scene
        m4x4 ModelTransform = model->get_transform_matrix();
        r32 min_hit = INFINITY;

        for (int tr = 0; tr < sb_count(model->triangles); ++tr) {
          Triangle triangle = model->triangles[tr];
          v3 vertices[3];
          for (int i = 0; i < 3; ++i) {
            vertices[i] =
                ModelTransform * model->vertices[triangle.vertices[i].index];
          }
          Triangle_Hit hit = ray.hits_triangle(vertices);
          if (hit.at > 0 && hit.at < min_hit) {
            min_hit = hit.at;
            state->selected_model = model;
          }
        }

        for (int f = 0; f < sb_count(model->fans); ++f) {
            Fan fan = model->fans[f];
            v3 vertices[Fan::kMaxNumVertices];
            // Transform all vertices
            for (int i = 0; i < fan.num_vertices; ++i) {
              vertices[i] =
                  ModelTransform * model->vertices[fan.vertices[i].index];
            }
            // Calculate hits
            for (int i = 0; i < fan.num_vertices - 2; ++i) {
              v3 triangle[3] = {vertices[0], vertices[i + 1], vertices[i + 2]};
              Triangle_Hit hit = ray.hits_triangle(triangle);
              if (hit.at > 0 && hit.at < min_hit) {
                min_hit = hit.at;
                state->selected_model = model;
              }
            }
          }
      }
    }
    if (input->key_went_down('A')) {
      Model model = state->models[0];
      model.position = ui->cursor;
      model.direction = (this->camera.position - model.position).normalized();
      model.transform_calculated = false;
      sb_push(state->models, model);
    }
    if (input->key_went_down('5')) {
      this->camera.ortho_projection = !this->camera.ortho_projection;
    } else if (input->key_went_down('1') || input->key_went_down('3') ||
               input->key_went_down('7')) {
      // View shortcuts
      v3 pivot = this->camera.pivot;
      r32 distance_to_pivot = this->camera.distance_to_pivot();
      this->camera.position = pivot;
      if (input->symbol == '1') {
        // Move to front
        this->camera.position_type = Camera_Position_Front;
        this->camera.position.z = pivot.z + distance_to_pivot;
        this->camera.up = V3(0, 1, 0);
      } else if (input->symbol == '3') {
        // Move to left
        this->camera.position_type = Camera_Position_Left;
        this->camera.position.x = pivot.x - distance_to_pivot;
        this->camera.up = V3(0, 1, 0);
      } else if (input->symbol == '7') {
        // Move to top
        this->camera.position_type = Camera_Position_Top;
        this->camera.position.y = pivot.y + distance_to_pivot;
        this->camera.up = V3(0, 1, -1);
      } else {
        assert(!"Wrong code path");
      }
      this->camera.look_at(pivot);
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
        this->camera.position_type = Camera_Position_User;
      }
    }
    if (input->button_is_down(IB_mouse_middle)) {
      v2 delta = V2(input->mouse_positions[IB_mouse_middle] - input->mouse);
      if (this->mode == Editor_3DView_Mode_Camera_Rotate) {
        this->camera.pivot = this->camera.old_pivot;
        // Rotate camera around pivot
        const int kSensitivity = 500;
        v2 angles = (M_PI / kSensitivity) * delta;
        angles.y = -angles.y;  // origin in bottom left so have to swap
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
        v3 move_vector = (right * delta.x + up * delta.y) * sensitivity;
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

  m4x4 ViewportTransform = Matrix::viewport(0, 0, area_width, area_height);

  m4x4 ClipSpaceTransform = ProjectionMatrix * CameraSpaceTransform;

  m4x4 WorldTransform = ViewportTransform * ClipSpaceTransform;

  // #pragma omp parallel for num_threads(2)
  for (int m = 0; m < sb_count(state->models); ++m) {
    Model *model = state->models + m;
    if (!model->display) continue;

    // Basic frustum culling - using the AABB calculated in the prev frame
    // (the vertices are already in the scene space)
    {
      if (model->old_position != model->position ||
          model->old_direction != model->direction) {
        model->old_direction = model->direction;
        model->old_position = model->position;
        model->update_aabb(true);  // transformed model aabb
      }
      v3 min = model->aabb.min;
      v3 max = model->aabb.max;
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

      // TODO: simd?
      bool all_outside_clipping_volume = true;
      for (int i = 0; i < 8; ++i) {
        v3 v = ClipSpaceTransform * verts[i];
        bool vertex_inside = (-1.0f <= v.x && v.x <= 1.0f) &&
                             (-1.0f <= v.y && v.y <= 1.0f) &&
                             (-1.0f <= v.z && v.z <= 1.0f);
        if (vertex_inside) {
          all_outside_clipping_volume = false;
          break;
        }
      }
      if (all_outside_clipping_volume) {
        continue;  // skip the model
      }
    }

    // Put model in the scene
    m4x4 ModelTransform = model->get_transform_matrix();

    v3 light_dir = -this->camera.direction;
    bool outline = (model == state->selected_model);

    for (int tr = 0; tr < sb_count(model->triangles); ++tr) {
      Triangle triangle = model->triangles[tr];
      v3 scene_verts[3];
      v3 screen_verts[3];
      // v2 texture_verts[3];
      v3 vns[3];

      for (int i = 0; i < 3; ++i) {
        scene_verts[i] =
            ModelTransform * model->vertices[triangle.vertices[i].index];

        screen_verts[i] = WorldTransform * scene_verts[i];
        vns[i] = model->vns[triangle.vertices[i].vn_index];
        vns[i] = V3(ModelTransform * V4_v(vns[i])).normalized();
      }

      triangle_rasterize_simd(area, screen_verts, vns, z_buffer, light_dir,
                              outline);
    }

    for (int f = 0; f < sb_count(model->fans); ++f) {
      Fan fan = model->fans[f];
      v3 screen_verts[Fan::kMaxNumVertices];
      v3 vns[Fan::kMaxNumVertices];

      for (int i = 0; i < fan.num_vertices; ++i) {
        v3 vertex = ModelTransform * model->vertices[fan.vertices[i].index];
        screen_verts[i] = WorldTransform * vertex;
        vns[i] = model->vns[fan.vertices[i].vn_index];
        vns[i] = V3(ModelTransform * V4_v(vns[i])).normalized();
      }

      for (int i = 0; i < fan.num_vertices - 2; ++i) {
        v3 triangle[3] = {screen_verts[0], screen_verts[i + 1],
                          screen_verts[i + 2]};
        v3 tr_vns[3] = {vns[0], vns[i + 1], vns[i + 2]};
        triangle_rasterize_simd(area, triangle, tr_vns, z_buffer, light_dir,
                                outline);
      }
    }

    if (model == state->selected_model) {
      v3 min = model->aabb.min;
      v3 max = model->aabb.max;
      // Draw the direction vector
      draw_line(area, WorldTransform * model->position,
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
        draw_line(area, WorldTransform * verts[lines[i]],
                  WorldTransform * verts[lines[i + 1]], 0x00FFAA40, z_buffer);
      }
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
        draw_line(area, vert1, vert2, color, z_buffer);
      }
      // Along the z axis
      {
        vert1 = WorldTransform * V3(step, 0.0f, -1.0);
        vert2 = WorldTransform * V3(step, 0.0f, 1.0);
        if (i == kLineCount / 2) {
          color = kZColor;
        }
        draw_line(area, vert1, vert2, color, z_buffer);
      }
    }
  }

  // Draw cursor
  {
    v2i cursor = V2i(WorldTransform * ui->cursor);
    Rect cursor_rect = {cursor.x - 3, cursor.y + 3, cursor.x + 3, cursor.y - 3};
    draw_rect(area, cursor_rect, 0x00FF0000);
  }

  // Draw the axis in the corner
  {
    m4x4 IconViewport = Matrix::viewport(5, 30, 60, 60);
    v3 x = V3(CameraSpaceTransform * V4(1, 0, 0, 0));
    v3 y = V3(CameraSpaceTransform * V4(0, 1, 0, 0));
    v3 z = V3(CameraSpaceTransform * V4(0, 0, 1, 0));

    v2i origin = V2i(IconViewport * V3(0, 0, 0));
    v2i X = V2i(IconViewport * x);
    v2i Y = V2i(IconViewport * y);
    v2i Z = V2i(IconViewport * z);

    draw_line(area, origin, X, kXColor, 2);
    draw_line(area, origin, Z, kZColor, 2);
    draw_line(area, origin, Y, kYColor, 2);

    v2i letter_adjust = V2i(-3, 17);
    draw_string(area, X + letter_adjust, "x", kXColor, false);
    draw_string(area, Y + letter_adjust, "y", kYColor, false);
    draw_string(area, Z + letter_adjust, "z", kZColor, false);
  }

  // Draw status string
  {
    char status_string[200];
    char *position_type;
    switch (this->camera.position_type) {
      case Camera_Position_User: {
        position_type = "User";
      } break;
      case Camera_Position_Front: {
        position_type = "Front";
      } break;
      case Camera_Position_Left: {
        position_type = "Left";
      } break;
      case Camera_Position_Top: {
        position_type = "Top";
      } break;
      default: { position_type = "User"; } break;
    };
    char *projection;
    if (this->camera.ortho_projection) {
      projection = "ortho";
    } else {
      projection = "persp";
    }
    sprintf(status_string, "%s %s", position_type, projection);
    draw_string(area, V2i(20, 10), status_string, 0x00FFFFFF, true, true);
  }

  // Display the name of the selected model
  if (state->selected_model != NULL) {
    draw_string(area, V2i(15, 50), state->selected_model->name, 0x00FFFFFF,
                false, true);
  }

#endif
}
