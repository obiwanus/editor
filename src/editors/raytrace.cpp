
void Editor_Raytrace::draw(Pixel_Buffer *buffer, Program_State *state) {
  Camera camera = this->area->editor_3dview.camera;

  int area_width = this->area->get_width();
  int area_height = this->area->get_height();

  Ray ray;
  m4x4 CameraSpaceTransform =
      Matrix::frame_to_canonical(camera.get_basis(), camera.position);
  ray.origin = CameraSpaceTransform * V3(0, 0, 0);

  // Get pixel in camera coordinates
  v2 pixel_size = V2(2 * camera.right / camera.viewport.x,
                     2 * camera.top / camera.viewport.y);
  v3 camera_pixel;
  camera_pixel.z = camera.near;

  for (int y = 0; y < area_height; ++y) {
    for (int x = 0; x < area_width; ++x) {
      camera_pixel.x = -camera.right + 0.5f * pixel_size.x + x * pixel_size.x;
      camera_pixel.y = -camera.top + 0.5f * pixel_size.y + y * pixel_size.y;
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
            draw_pixel(this->area, x, y, 0x00FFFFFF);
          }
        }
      }
    }
  }
}
