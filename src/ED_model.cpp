
void Model::read_texture(char *filename) {
  Image image = {};
  image.data = (u32 *)stbi_load(filename, &image.width, &image.height,
                                &image.bytes_per_pixel, 4);
  if (image.bytes_per_pixel < 3 || image.bytes_per_pixel > 4) {
    printf("Image format not supported: %s\n", filename);
    exit(1);
  }
  if (image.data == NULL) {
    printf("Can't read texture file %s\n", filename);
    exit(1);
  }
  this->texture = image;
}

void Model::update_aabb(bool transformed) {
  v3 min = V3(INFINITY, INFINITY, INFINITY);
  v3 max = V3(-INFINITY, -INFINITY, -INFINITY);
  m4x4 Transform = Matrix::frame_to_canonical(this->get_basis(), V3(0, 0, 0)) *
                   Matrix::S(this->scale);
  for (int i = 0; i < sb_count(this->vertices); ++i) {
    v3 vertex = this->vertices[i];
    if (transformed) {
      vertex = Transform * vertex;
    }

    for (int j = 0; j < 3; ++j) {
      r32 value = vertex.E[j];
      if (value < min.E[j]) {
        min.E[j] = value;
      } else if (max.E[j] < value) {
        max.E[j] = value;
      }
    }
  }
  this->aabb.min = min + this->position;
  this->aabb.max = max + this->position;
}

void Model::set_defaults() {
  this->triangles = NULL;
  this->vertices = NULL;
  this->vts = NULL;
  this->vns = NULL;
  this->scale = 1.0f;
  this->direction = V3(0, 0, 1);
  this->display = true;
  this->debug = false;
}

void Model::destroy() {
  sb_free(this->vertices);
  sb_free(this->vns);
  sb_free(this->vts);
  sb_free(this->triangles);
}

m4x4 Model::get_transform_matrix() {
  if (this->transform_calculated) {
    return this->TransformMatrix;
  }
  this->TransformMatrix =
      Matrix::frame_to_canonical(this->get_basis(), this->position) *
      Matrix::S(this->scale);
  this->transform_calculated = true;
  return this->TransformMatrix;
}

m4x4 Entity::transform_to_entity_space() {
  m4x4 result;
  basis3 basis = this->get_basis();
  result = Matrix::canonical_to_frame(basis, this->position);
  return result;
}

basis3 Entity::get_basis() {
  assert(direction.x != 0 || direction.y != 0 || direction.z != 0);
  assert(up.x != 0 || up.y != 0 || up.z != 0);

  basis3 result;
  result.w = this->direction.normalized();
  result.u = this->up.cross(result.w).normalized();
  result.v = result.w.cross(result.u);
  return result;
}

v3 Ray::get_point_at(r32 t) {
  v3 result = this->origin + this->direction * t;
  return result;
}

Ray Camera::get_ray_through_pixel(v2i pixel) {
  Ray result;

  // Get pixel in camera coordinates
  v2 pixel_size = V2(2 * right / viewport.x, 2 * top / viewport.y);
  v3 camera_pixel;
  camera_pixel.x = -right + (pixel.x + 0.5f) * pixel_size.x;
  camera_pixel.y = -top + (pixel.y + 0.5f) * pixel_size.y;
  camera_pixel.z = this->near;

  if (this->ortho_projection) {
    result.origin = camera_pixel;
    result.direction = V3(0, 0, -1);
  } else {
    result.origin = V3(0, 0, 0);
    result.direction = camera_pixel - result.origin;
  }

  // Transform into world coordinates
  m4x4 WorldTransform =
      Matrix::frame_to_canonical(this->get_basis(), this->position);
  result.origin = WorldTransform * result.origin;
  result.direction = V3(WorldTransform * V4_v(result.direction));

  return result;
}

void Camera::adjust_frustum(int width, int height) {
  assert(width > 0 && height > 0);
  this->viewport = V2i(width, height);
  r32 aspect_ratio = (r32)width / (r32)height;
  // Vertical field of view is fixed at 60 degrees
  const r32 kVerticalFOV = M_PI / 3;
  this->top = abs(this->near) * (r32)tan(kVerticalFOV / 2);
  this->right = this->top * aspect_ratio;
}

void Camera::look_at(v3 point) {
  this->pivot = point;
  this->direction = -(point - this->position).normalized();
}

m4x4 Camera::projection_matrix() {
  m4x4 result;
  assert(this->near != 0);
  if (this->ortho_projection) {
    r32 ratio = this->distance_to_pivot() / abs(this->near);
    r32 t = this->top * ratio;
    r32 r = this->right * ratio;
    result = Matrix::ortho_projection(-r, r, -t, t, near, far);
  } else {
    result = Matrix::persp_projection(-right, right, -top, top, near, far);
  }
  return result;
}

m4x4 Camera::rotation_matrix(v2 angles) {
  // NOTE: this function assumes this->old_basis had been computed!
  basis3 original_basis = {V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1)};
  m4x4 Horizontal = Matrix::frame_to_canonical(original_basis, this->pivot) *
                    (this->old_up.y > 0 ? Matrix::Ry(angles.x)
                                        : Matrix::Ry(angles.x).transposed()) *
                    Matrix::canonical_to_frame(original_basis, this->pivot);

  m4x4 Vertical = Matrix::frame_to_canonical(this->old_basis, this->pivot) *
                  Matrix::Rx(angles.y) *
                  Matrix::canonical_to_frame(this->old_basis, this->pivot);

  m4x4 result = Horizontal * Vertical;
  return result;
}

r32 Camera::distance_to_pivot() {
  r32 result = (this->pivot - this->position).len();
  return result;
}

Triangle_Hit Ray::hits_triangle(v3 vertices[]) {
  Triangle_Hit result;

  v3 a = vertices[0];
  v3 b = vertices[1];
  v3 c = vertices[2];

  // clang-format off
  m3x3 A = {
    a.x - b.x, a.x - c.x, this->direction.x,
    a.y - b.y, a.y - c.y, this->direction.y,
    a.z - b.z, a.z - c.z, this->direction.z,
  };
  // clang-format on

  r32 D = A.determinant();
  if (D == 0) {
    result.at = -1;
    return result;
  }
  v3 R = {a.x - this->origin.x, a.y - this->origin.y, a.z - this->origin.z};

  // Use Cramer's rule to find t, beta, and gamma
  m3x3 A2 = A.replace_column(2, R);
  r32 t = A2.determinant() / D;
  if (t <= 0) {
    result.at = -1;
    return result;
  }
  r32 gamma = A.replace_column(1, R).determinant() / D;
  if (gamma < 0 || gamma > 1) {
    result.at = -1;
    return result;
  }
  r32 beta = A.replace_column(0, R).determinant() / D;
  if (beta < 0 || beta > (1 - gamma)) {
    result.at = -1;
    return result;
  }

  result.at = t;
  result.barycentric[0] = 1.0f - beta - gamma;
  result.barycentric[1] = beta;
  result.barycentric[2] = gamma;
  return result;
}

bool Ray::hits_aabb(AABBox aabb) {
  v3 ray_inv_direction = 1.0f / this->direction;

  v3 t1 = (aabb.min - this->origin).hadamard(ray_inv_direction);
  v3 t2 = (aabb.max - this->origin).hadamard(ray_inv_direction);

  r32 tmin = max3(min(t1.x, t2.x), min(t1.y, t2.y), min(t1.z, t2.z));
  r32 tmax = min3(max(t1.x, t2.x), max(t1.y, t2.y), max(t1.z, t2.z));

  return tmax >= tmin && tmax > 0;
}
