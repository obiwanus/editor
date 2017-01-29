
void Editor_Raytrace::draw(Pixel_Buffer *buffer, Program_State *state,
                           User_Input *input) {
  int area_width = this->area->get_width();
  int area_height = this->area->get_height();

  if (this->is_drawn) {
    if (input->button_went_down(IB_escape)) {
      this->area->editor_type = Area_Editor_Type_3DView;
      this->area->type_select.option_selected = this->area->editor_type;
    }

    TIMED_BLOCK();
    // Blit the contents of the back buffer
    // TODO: simd?
    v2i start, end;
    start.x = max(0, (this->backbuffer.width - area_width) / 2);
    start.y = max(0, (this->backbuffer.height - area_height) / 2);
    end.x = start.x + min(this->backbuffer.width, area_width);
    end.y = start.y + min(this->backbuffer.height, area_height);
    for (int y = start.y; y < end.y; ++y) {
      for (int x = start.x; x < end.x; ++x) {
        u32 *pixel_src =
            (u32 *)this->backbuffer.memory + y * this->backbuffer.width + x;
        int x_dst =
            this->area->left + (area_width - this->backbuffer.width) / 2 + x;
        int y_dst = buffer->height - this->area->top +
                    (area_height - this->backbuffer.height) / 2 + y;
        u32 *pixel_dst = (u32 *)buffer->memory + y_dst * buffer->width + x_dst;
        *pixel_dst = *pixel_src;
      }
    }
    return;
  }

  // Draw
  this->is_drawn = true;

  // Always update the boundaries when drawing
  this->backbuffer.width = area_width;
  this->backbuffer.height = area_height;

  int bb_size = area_width * area_height * sizeof(u32);

  if (this->backbuffer.memory == NULL) {
    // Allocate back buffer
    this->backbuffer.max_width = area_width;
    this->backbuffer.max_height = area_height;
    this->backbuffer.memory = malloc(bb_size);
  } else if (this->backbuffer.max_width < area_width ||
             this->backbuffer.max_height < area_height) {
    // Reallocate since we have a bigger area now
    this->backbuffer.max_width = area_width;
    this->backbuffer.max_height = area_height;
    this->backbuffer.memory = realloc(this->backbuffer.memory, bb_size);
  }

  // Clear (maybe temporary)
  memset(this->backbuffer.memory, EDITOR_BACKGROUND_COLOR, bb_size);

  // this->trace_tile(state->models, V2i(0, 0), V2i(area_width, area_height));

  const int kTileCount = 4;  // one side
  v2i tile_size = {area_width / kTileCount, area_height / kTileCount};
  for (int y = 0; y < kTileCount; ++y) {
    for (int x = 0; x < kTileCount; ++x) {
      v2i start = {x * tile_size.x, y * tile_size.y};
      v2i end = start + tile_size;
      Raytrace_Work_Entry entry;
      entry.editor = this;
      entry.models = state->models;
      entry.start = start;
      entry.end = end;
      state->raytrace_queue->add_entry(entry);
    }
  }
}

void Editor_Raytrace::trace_tile(Model *models, v2i start, v2i end) {
  Camera camera = this->area->editor_3dview.camera;

  Ray ray;
  m4x4 CameraSpaceTransform =
      Matrix::frame_to_canonical(camera.get_basis(), camera.position);
  ray.origin = CameraSpaceTransform * V3(0, 0, 0);

  // Get pixel in camera coordinates
  v2 pixel_size = V2(2 * camera.right / camera.viewport.x,
                     2 * camera.top / camera.viewport.y);

  r32 x_start = -camera.right + pixel_size.x * (0.5f + start.x);

  v3 camera_pixel;
  camera_pixel.x = x_start;
  camera_pixel.y = -camera.top + pixel_size.y * (0.5f + start.y);
  camera_pixel.z = camera.near;

  for (int y = start.y; y < end.y; ++y) {
    for (int x = start.x; x < end.x; ++x) {
      ray.direction = CameraSpaceTransform * camera_pixel - ray.origin;

      {
        Triangle_Hit triangle_hit;
        triangle_hit.at = INFINITY;
        int model_id = -1;
        int object_id = -1;
        int fan_triangle_id = -1;
        Object_Type object_type;

        for (int m = 0; m < sb_count(models); ++m) {
          Model *model = models + m;
          if (!model->display) continue;
          if (!ray.hits_aabb(model->aabb)) continue;

          // Put model in the scene
          m4x4 ModelTransform = model->get_transform_matrix();

          // Look at triangles
          for (int tr = 0; tr < sb_count(model->triangles); ++tr) {
            Triangle triangle = model->triangles[tr];
            v3 vertices[3];
            for (int i = 0; i < 3; ++i) {
              vertices[i] =
                  ModelTransform * model->vertices[triangle.vertices[i].index];
            }
            Triangle_Hit hit = ray.hits_triangle(vertices);
            if (hit.at > 0 && hit.at < triangle_hit.at) {
              triangle_hit = hit;
              model_id = m;
              object_id = tr;
              object_type = Object_Type_Triangle;
            }
          }

          // Look at fans
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
              if (hit.at > 0 && hit.at < triangle_hit.at) {
                triangle_hit = hit;
                model_id = m;
                object_id = f;
                object_type = Object_Type_Fan;
                fan_triangle_id = i;
              }
            }
          }
        }

        // See if we hit anything
        if (model_id >= 0 && object_id >= 0) {
          v3 light_source = V3(-1, 2, 3);
          v3 normal = {};
          v3 hit_point = ray.get_point_at(triangle_hit.at);
          v3 light_dir = (light_source - hit_point).normalized();
          Model *model = models + model_id;
          m4x4 ModelTransform = model->get_transform_matrix();
          if (object_type == Object_Type_Triangle) {
            Triangle triangle = model->triangles[object_id];
            for (int i = 0; i < 3; ++i) {
              normal += model->vns[triangle.vertices[i].vn_index] *
                        triangle_hit.barycentric[i];
            }
          } else if (object_type == Object_Type_Fan) {
            Fan fan = model->fans[object_id];
            int vertex_ids[3] = {0, fan_triangle_id + 1, fan_triangle_id + 2};
            for (int i = 0; i < 3; ++i) {
              normal += model->vns[fan.vertices[vertex_ids[i]].vn_index] *
                        triangle_hit.barycentric[i];
            }
          }
          normal = V3(ModelTransform * V4_v(normal.normalized()));
          r32 intensity = light_dir * normal;
          if (intensity < 0) intensity = 0;
          intensity = lerp(0.2f, 1.0f, intensity);
          u32 color = get_rgb_u32(V3(0.7f, 0.7f, 0.7f) * intensity);
          draw_pixel(&this->backbuffer, x, y, color);
        }
      }

      camera_pixel.x += pixel_size.x;
    }
    camera_pixel.x = x_start;
    camera_pixel.y += pixel_size.y;

    // This is a way to abort ray trace if the editor type has changed
    if (this->area->editor_type != Area_Editor_Type_Raytrace) {
      return;
    }
  }
}

// v3 Ray::get_color(ProgramState *state, RayObject *reflected_from,
//                   int recurse_further) {
//   Ray *ray = this;

//   v3 color = {};

//   RayHit ray_hit = ray->get_object_hit(state, 0, INFINITY, reflected_from);

//   if (ray_hit.object == NULL) {
//     color = {0.05f, 0.05f, 0.05f};  // background color
//     return color;
//   }

//   v3 hit_point = ray->point_at(ray_hit.at);
//   v3 normal = ray_hit.object->get_normal(hit_point);
//   v3 line_of_sight = -ray->direction.normalized();

//   for (int i = 0; i < state->kLightCount; i++) {
//     LightSource *light = &state->lights[i];

//     v3 light_direction = (hit_point - light->source).normalized();

//     b32 point_in_shadow = false;
//     {
//       // Cast shadow ray
//       Ray shadow_ray = Ray();
//       shadow_ray.origin = hit_point;
//       shadow_ray.direction =
//           light->source - hit_point;  // not normalizing on purpose

//       RayHit shadow_ray_hit =
//           shadow_ray.get_object_hit(state, 0, 1, ray_hit.object, true);
//       if (shadow_ray_hit.object != NULL) {
//         point_in_shadow = true;
//       }
//     }

//     if (!point_in_shadow) {
//       v3 V = (-light_direction + line_of_sight).normalized();

//       r32 illuminance = -light_direction * normal;
//       if (illuminance < 0) {
//         illuminance = 0;
//       }
//       color += ray_hit.object->color * light->intensity * illuminance;

//       // Calculate specular reflection
//       r32 reflection = V * normal;
//       if (reflection < 0) {
//         reflection = 0;
//       }
//       v3 specular_reflection = ray_hit.object->specular_color *
//                                light->intensity *
//                                (r32)pow(reflection,
//                                ray_hit.object->phong_exp);
//       color += specular_reflection;
//     }
//   }

//   // Calculate mirror reflection
//   if (recurse_further) {
//     Ray reflection_ray = {};
//     reflection_ray.origin = hit_point;
//     reflection_ray.direction =
//         ray->direction - 2 * (ray->direction * normal) * normal;

//     const r32 max_k = 0.3f;
//     r32 k = ray_hit.object->phong_exp * 0.001f;
//     if (k > max_k) {
//       k = max_k;
//     }
//     color +=
//         k *
//         reflection_ray.get_color(state, ray_hit.object, recurse_further - 1);
//   }

//   return color;
// }
