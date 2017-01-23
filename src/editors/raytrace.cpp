
void Editor_Raytrace::draw(Pixel_Buffer *buffer, Program_State *state) {

  int area_width = this->area->get_width();
  int area_height = this->area->get_height();

  if (this->is_drawn) {
    // Blit the contents of the back buffer
    // TODO: simd?
    v2i start, end;
    start.x = max(0, (this->backbuffer.width - area_width) / 2);
    start.y = max(0, (this->backbuffer.height - area_height) / 2);
    end.x = start.x + min(this->backbuffer.width, area_width);
    end.y = start.y + min(this->backbuffer.height, area_height);
    for (int y = start.y; y < end.y; ++y) {
      for (int x = start.x; x < end.x; ++x) {
        u32 *pixel_src = (u32 *)this->backbuffer.memory + y * this->backbuffer.width + x;
        int x_dst = this->area->left + (area_width - this->backbuffer.width) / 2 + x;
        int y_dst = buffer->height - this->area->top + (area_height - this->backbuffer.height) / 2 + y;
        u32 *pixel_dst = (u32 *)buffer->memory + y_dst * buffer->width + x_dst;
        *pixel_dst = *pixel_src;
      }
    }
    return;
  }

  // Draw. TODO: use at least 1 separate thread
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

  Camera camera = this->area->editor_3dview.camera;

  Ray ray;
  m4x4 CameraSpaceTransform =
      Matrix::frame_to_canonical(camera.get_basis(), camera.position);
  ray.origin = CameraSpaceTransform * V3(0, 0, 0);

  // Get pixel in camera coordinates
  v2 pixel_size = V2(2 * camera.right / camera.viewport.x,
                     2 * camera.top / camera.viewport.y);

  r32 x_start = -camera.right + 0.5f * pixel_size.x;  // x = 0

  v3 camera_pixel;
  camera_pixel.x = x_start;
  camera_pixel.y = -camera.top + 0.5f * pixel_size.y;  // y = 0
  camera_pixel.z = camera.near;

  for (int y = 0; y < area_height; ++y) {
    for (int x = 0; x < area_width; ++x) {
      ray.direction = camera_pixel;  // we're assuming ray origin is 0, 0, 0
      ray.direction = V3(CameraSpaceTransform * V4_v(ray.direction));

      for (int m = 0; m < sb_count(state->models); ++m) {
        Model *model = state->models + m;
        if (!model->display) continue;

        // Put model in the scene
        m4x4 ModelTransform =
            Matrix::frame_to_canonical(model->get_basis(), model->position) *
            Matrix::S(model->scale);

        for (int tr = 0; tr < sb_count(model->triangles); ++tr) {
          Triangle triangle = model->triangles[tr];
          v3 vertices[3];
          for (int i = 0; i < 3; ++i) {
            vertices[i] =
                ModelTransform * model->vertices[triangle.vertices[i].index];
          }
          if (ray.hits_triangle(vertices) > 0) {
            draw_pixel(&this->backbuffer, x, y, 0x00FFFFFF);
          }
        }
      }

      camera_pixel.x += pixel_size.x;
    }
    camera_pixel.x = x_start;
    camera_pixel.y += pixel_size.y;
  }
}
