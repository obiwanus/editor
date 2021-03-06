
r32 v2i::len() { return (r32)sqrt(x * x + y * y); }

r32 v2::len() { return (r32)sqrt(x * x + y * y); }

r32 v3::len() { return (r32)sqrt(x * x + y * y + z * z); }

v2 v2::normalized() {
  r32 len = this->len();
  if (len == 0) return V2(0, 0);
  v2 result = *this / len;
  return result;
}

v3 v3::normalized() {
  r32 len = this->len();
  if (len == 0) return V3(0, 0, 0);
  v3 result = *this / len;
  return result;
}

v3 v3::cross(v3 Vector) {
  v3 result;

  result.x = y * Vector.z - z * Vector.y;
  result.y = z * Vector.x - x * Vector.z;
  result.z = x * Vector.y - y * Vector.x;

  return result;
}

v3 v3::hadamard(v3 Vector) {
  v3 result;

  result.x = x * Vector.x;
  result.y = y * Vector.y;
  result.z = z * Vector.z;

  return result;
}

v4 v4::homogenized() {
  v4 result;

  if (this->w == 0 && this->w == 1) {
    return *this;
  }
  result.x = this->x / this->w;
  result.y = this->y / this->w;
  result.z = this->z / this->w;
  result.w = 1;

  return result;
}

r32 m2x2::determinant() {
  r32 result = a * b - c * d;

  return result;
}

r32 m3x3::determinant() {
  r32 result =
      a * e * i - a * h * f - b * d * i + b * f * g + c * d * h - c * e * g;

  return result;
}

m3x3 m3x3::replace_column(int number, v3 column) {
  m3x3 result = *this;

  result.E[number + 0] = column.E[0];
  result.E[number + 3] = column.E[1];
  result.E[number + 6] = column.E[2];

  return result;
}

m3x3 m3x3::transposed() {
  // clang-format off
  m3x3 result = {
    a, d, g,
    b, e, h,
    c, f, i,
  };
  // clang-format on
  return result;
}

v4 m4x4::column(int number) {
  v4 result;

  for (int row = 0; row < 4; ++row) {
    result.E[row] = this->E[4 * row + number];
  }

  return result;
}

m4x4 m4x4::transposed() {
  // clang-format off
  m4x4 result = {
    m11, m21, m31, m41,
    m12, m22, m32, m42,
    m13, m23, m33, m43,
    m14, m24, m34, m44,
  };
  // clang-format on
  return result;
}

basis3 basis3::build_from(v3 r) {
  // Builds a right-handed basis with vector r in the direction of x
  basis3 result;
  assert(r != V3(0, 0, 0));

  result.r = r.normalized();
  if (abs(r.x) <= abs(r.y) && abs(r.x) <= abs(r.z)) {
    result.s = V3(0.0f, -r.z, r.y);
  } else if (abs(r.y) <= abs(r.x) && abs(r.y) <= abs(r.z)) {
    result.s = V3(-r.z, 0.0f, r.x);
  } else {
    result.s = V3(-r.y, r.x, 0.0f);
  }
  result.s = result.s.normalized();
  result.t = result.r.cross(result.s);

  return result;
}

m4x4 Matrix::identity() {
  // clang-format off
  m4x4 result = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };
  // clang-format on
  return result;
}

m4x4 Matrix::ortho_projection(r32 l, r32 r, r32 b, r32 t, r32 n, r32 f) {
  assert(n > f && l < r && b < t);

  // This matrix assumes negative n and f. If they were positive,
  // the values in the third row should have opposite sign
  assert(n < 0);

  // clang-format off
  m4x4 result = {
    2.0f/(r-l), 0,          0,          -(r+l)/(r-l),
    0,          2.0f/(t-b), 0,          -(t+b)/(t-b),
    0,          0,         -2.0f/(f-n),  (f+n)/(f-n),
    0,          0,          0,           1,
  };
  // clang-format on

  return result;
}

m4x4 Matrix::persp_projection(r32 l, r32 r, r32 b, r32 t, r32 n, r32 f) {
  assert(n > f && l < r && b < t);

  // This matrix assumes negative n and f. If they were positive,
  // the values in the third row should have opposite sign
  assert(n < 0);

  // clang-format off
  m4x4 result = {
    2.0f*n/(r-l), 0,            -(r+l)/(r-l),  0,
    0,            2.0f*n/(t-b), -(t+b)/(t-b),  0,
    0,            0,            -(f+n)/(f-n),  2.0f*f*n/(f-n),
    0,            0,             1,            0,
  };
  // clang-format on

  return result;
}

m4x4 Matrix::Rx(r32 angle) {
  r32 c = (r32)cos(angle);
  r32 s = (r32)sin(angle);
  // clang-format off
  m4x4 result = {
    1, 0,  0, 0,
    0, c, -s, 0,
    0, s,  c, 0,
    0, 0,  0, 1,
  };
  //clang-format on
  return result;
}

m4x4 Matrix::Ry(r32 angle) {
  r32 c = (r32)cos(angle);
  r32 s = (r32)sin(angle);
  // clang-format off
  m4x4 result = {
    c, 0, s, 0,
    0, 1, 0, 0,
   -s, 0, c, 0,
    0, 0, 0, 1,
  };
  //clang-format on
  return result;
}

m4x4 Matrix::Rz(r32 angle) {
  r32 c = (r32)cos(angle);
  r32 s = (r32)sin(angle);
  // clang-format off
  m4x4 result = {
    c, -s,  0,  0,
    s,  c,  0,  0,
    0,  0,  1,  0,
    0,  0,  0,  1,
  };
  //clang-format on
  return result;
}

m4x4 Matrix::R(v3 axis, v3 angle, v3 point) {
  m4x4 result = {};

  // TODO

  return result;
}

m4x4 Matrix::S(r32 sx, r32 sy, r32 sz) {
  // clang-format off
  m4x4 result = {
    sx,  0,  0,  0,
     0, sy,  0,  0,
     0,  0, sz,  0,
     0,  0,  0,  1,
  };
  // clang-format on
  return result;
}

m4x4 Matrix::S(r32 s) {
  // Uniform scaling
  return Matrix::S(s, s, s);
}

m4x4 Matrix::T(r32 tx, r32 ty, r32 tz) {
  // clang-format off
  m4x4 result = {
    1,  0,  0, tx,
    0,  1,  0, ty,
    0,  0,  1, tz,
    0,  0,  0,  1,
  };
  // clang-format on
  return result;
}

m4x4 Matrix::T(v3 t) {
  return Matrix::T(t.x, t.y, t.z);
}

m4x4 Matrix::frame_to_canonical(basis3 frame, v3 origin) {
  // clang-format off
  m4x4 result = {
    frame.u.x, frame.v.x, frame.w.x, origin.x,
    frame.u.y, frame.v.y, frame.w.y, origin.y,
    frame.u.z, frame.v.z, frame.w.z, origin.z,
            0,         0,         0,        1,
  };
  // clang-format on
  return result;
}

m4x4 Matrix::canonical_to_frame(basis3 frame, v3 origin) {
  m4x4 result;
  // clang-format off
  m4x4 rotate_inverse = {
    frame.u.x, frame.u.y, frame.u.z,  0,
    frame.v.x, frame.v.y, frame.v.z,  0,
    frame.w.x, frame.w.y, frame.w.z,  0,
            0,         0,         0,  1,
  };
  // clang-format on
  m4x4 Translate = Matrix::T(-origin);
  result = rotate_inverse * Translate;
  return result;
}

m4x4 Matrix::viewport(int x, int y, int width, int height, int z_depth) {
  r32 w = (r32)width;
  r32 h = (r32)height;
  r32 z = (r32)z_depth;

  // clang-format off
  m4x4 result = {
    w/2, 0,   0,   x + w/2,
    0,   h/2, 0,   y + h/2,
    0,   0,   z/2, z/2,
    0,   0,   0,   1,
  };
  // clang-format on
  return result;
}
