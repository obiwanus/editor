#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#include <math.h>
#include "ED_base.h"

#ifndef M_PI
#define M_PI (r32)acos(-1.0)
#endif // M_PI

// ================ Misc ======================

inline r32 abs(r32 value) {
  assert(value > 0 || value <= 0);  // catch NaNs
  return value > 0 ? value : -value;
}

inline i32 round_i32(r32 value) {
  i32 result = (int)(value + 0.5f);
  return result;
}

// ================ Vectors ======================

struct v2i {
  int x;
  int y;

  r32 len();
};

struct v3i {
  int x;
  int y;
  int z;

  r32 len();
};

union v2 {
  struct {
    r32 x, y;
  };
  struct {
    r32 u, v;
  };
  r32 E[2];

  r32 len();
  v2 normalized();
};

union v3 {
  struct {
    r32 x, y, z;
  };
  struct {
    r32 u, v, w;
  };
  struct {
    r32 r, g, b;
  };
  struct {
    v2 xy;
    r32 _ignored0;
  };
  struct {
    r32 _ignored1;
    v2 yz;
  };
  struct {
    v2 uv;
    r32 _ignored2;
  };
  struct {
    r32 _ignored3;
    v2 vw;
  };
  r32 E[3];

  r32 len();
  v3 normalized();
  v3 cross(v3 Vector);
};

union v4 {
  struct {
    r32 x, y, z, w;
  };
  // struct {
  //   r32 u, v, w;
  // };
  struct {
    r32 r, g, b, a;
  };
  r32 E[4];

  v4 homogenized();
};

union basis3 {
  struct {
    v3 vectors[3];
  };
  struct {
    v3 vect0, vect1, vect2;
  };
  struct {
    v3 u, v, w;
  };
  struct {
    v3 r, s, t;
  };
  struct {
    r32 x0, y0, z0;
    r32 x1, y1, z1;
    r32 x2, y2, z2;
  };
  r32 E[9];

  static basis3 build_from(v3);
};

// =========================== Matrices ======================================

union m2x2 {
  struct {
    v2 rows[2];
  };
  struct {
    r32 a, b;
    r32 c, d;
  };
  r32 E[4];

  r32 determinant();
};

union m3x3 {
  struct {
    v3 rows[3];
  };
  struct {
    r32 a, b, c;
    r32 d, e, f;
    r32 g, h, i;
  };
  r32 E[9];

  r32 determinant();
  m3x3 replace_column(int number, v3 column);
  m3x3 transposed();
};

union m4x4 {
  struct {
    v4 rows[4];
  };
  struct {
    r32 a, b, c, d;
    r32 e, f, g, h;
    r32 i, j, k, l;
    r32 m, n, o, p;
  };
  r32 E[16];
  v4 column(int);
  m4x4 transposed();
};

namespace Matrix {
  // Rotation
  m4x4 Rx(r32 angle);
  m4x4 Ry(r32 angle);
  m4x4 Rz(r32 angle);
  m4x4 R(v3 axis, v3 angle, v3 point = {0, 0, 0});
  // Scaling
  m4x4 S(r32);
  m4x4 S(r32, r32, r32);
  // Translation
  m4x4 T(r32, r32, r32);
  m4x4 T(v3);
  m4x4 viewport(int, int, int, int);
  // Projection
  m4x4 ortho_projection(r32, r32, r32, r32, r32, r32);
  m4x4 persp_projection(r32, r32, r32, r32, r32, r32);
  // Change frame
  m4x4 frame_to_canonical(basis3, v3);
  m4x4 canonical_to_frame(basis3, v3);
}

// ==================== Construction ======================

inline v2i V2i(i32 X, i32 Y) {
  v2i result = {(int)X, (int)Y};

  return result;
}

inline v2i V2i(u32 X, u32 Y) {
  v2i result = {(int)X, (int)Y};

  return result;
}

inline v2i V2i(r32 X, r32 Y) {
  v2i result;

  result.x = round_i32(X);
  result.y = round_i32(Y);

  return result;
}

inline v2i V2i(v2 A) {
  v2i result;

  result.x = round_i32(A.x);
  result.y = round_i32(A.y);

  return result;
}

inline v2i V2i(v3i A) {
  v2i result;

  result.x = A.x;
  result.y = A.y;

  return result;
}

inline v2i V2i(v3 A) {
  v2i result;

  result.x = round_i32(A.x);
  result.y = round_i32(A.y);

  return result;
}

inline v3i V3i(v3 A) {
  v3i result;

  result.x = round_i32(A.x);
  result.y = round_i32(A.y);
  result.z = round_i32(A.z);

  return result;
}

inline v2 V2(v3 A) {
  v2 result = {A.x, A.y};

  return result;
}

inline v2 V2(i32 X, i32 Y) {
  v2 result = {(r32)X, (r32)Y};

  return result;
}

inline v2 V2(u32 X, u32 Y) {
  v2 result = {(r32)X, (r32)Y};

  return result;
}

inline v2 V2(r32 X, r32 Y) {
  v2 result;

  result.x = X;
  result.y = Y;

  return result;
}

inline v3 V3(v3i A) {
  v3 result;

  result.x = (r32)A.x;
  result.y = (r32)A.y;
  result.z = (r32)A.z;

  return result;
}

inline v3 V3(r32 X, r32 Y, r32 Z) {
  v3 result;

  result.x = X;
  result.y = Y;
  result.z = Z;

  return result;
}

inline v3 V3(i32 X, i32 Y, i32 Z) {
  v3 result = {(r32)X, (r32)Y, (r32)Z};

  return result;
}

inline v3 V3(u32 X, u32 Y, u32 Z) {
  v3 result = {(r32)X, (r32)Y, (r32)Z};

  return result;
}

inline v3 V3(v2 XY, r32 Z) {
  v3 result;

  result.x = XY.x;
  result.y = XY.y;
  result.z = Z;

  return result;
}

// ========================== scalar operations ====================

inline r32 square(r32 A) {
  return A * A;
}

// ========================== v2 operations ========================

inline v2i operator+(v2i A, v2i B) {
  v2i result;

  result.x = A.x + B.x;
  result.y = A.y + B.y;

  return result;
}

inline v2i operator-(v2i A, v2i B) {
  v2i result;

  result.x = A.x - B.x;
  result.y = A.y - B.y;

  return result;
}

inline v2i operator*(v2i A, r32 scalar) {
  v2i result;

  result.x = round_i32(scalar * A.x);
  result.y = round_i32(scalar * A.y);

  return result;
}

inline v2 operator*(r32 A, v2 B) {
  v2 result;

  result.x = A * B.x;
  result.y = A * B.y;

  return result;
}

inline v2 operator/(v2 A, r32 B) {
  v2 result;

  result.x = A.x / B;
  result.y = A.y / B;

  return result;
}

inline v2 operator*(v2 B, r32 A) {
  v2 result = A * B;

  return result;
}

inline v2 &operator*=(v2 &B, r32 A) {
  B = A * B;

  return B;
}

inline v2 operator-(v2 A) {
  v2 result;

  result.x = -A.x;
  result.y = -A.y;

  return result;
}

inline v2 operator+(v2 A, v2 B) {
  v2 result;

  result.x = A.x + B.x;
  result.y = A.y + B.y;

  return result;
}

inline v2 &operator+=(v2 &A, v2 B) {
  A = A + B;

  return A;
}

inline v2 operator-(v2 A, v2 B) {
  v2 result;

  result.x = A.x - B.x;
  result.y = A.y - B.y;

  return result;
}

inline r32 operator*(v2 A, v2 B) {
  r32 result = A.x * B.x + A.y * B.y;

  return result;
}

inline bool operator==(v2 A, v2 B) {
  bool result = (A.x == B.x && A.y == B.y);

  return result;
}

inline bool operator!=(v2 A, v2 B) {
  bool result = (A.x != B.x || A.y != B.y);

  return result;
}

// ============================= v3 operations =======================

inline v3 operator*(r32 A, v3 B) {
  v3 result;

  result.x = A * B.x;
  result.y = A * B.y;
  result.z = A * B.z;

  return result;
}

inline v3 operator*(v3 B, r32 A) {
  v3 result = A * B;

  return result;
}


inline v3 operator/(v3 A, r32 B) {
  v3 result;

  result.x = A.x / B;
  result.y = A.y / B;
  result.z = A.z / B;

  return result;
}

inline r32 operator*(v3 A, v3 B) {
  r32 result = A.x * B.x + A.y * B.y + A.z * B.z;

  return result;
}

inline v3 &operator*=(v3 &B, r32 A) {
  B = A * B;

  return B;
}

inline v3 operator-(v3 A) {
  v3 result;

  result.x = -A.x;
  result.y = -A.y;
  result.z = -A.z;

  return result;
}

inline v3i operator+(v3i A, v3i B) {
  v3i result;

  result.x = A.x + B.x;
  result.y = A.y + B.y;
  result.z = A.z + B.z;

  return result;
}

inline v3 operator+(v3 A, v3 B) {
  v3 result;

  result.x = A.x + B.x;
  result.y = A.y + B.y;
  result.z = A.z + B.z;

  return result;
}

inline v3 &operator+=(v3 &A, v3 B) {
  A = A + B;

  return A;
}

inline v3i operator-(v3i A, v3i B) {
  v3i result;

  result.x = A.x - B.x;
  result.y = A.y - B.y;
  result.z = A.z - B.z;

  return result;
}

inline v3 operator-(v3 A, v3 B) {
  v3 result;

  result.x = A.x - B.x;
  result.y = A.y - B.y;
  result.z = A.z - B.z;

  return result;
}

inline v3 operator*(m3x3 M, v3 V) {
  v3 result = {};

  result.x = M.rows[0].x * V.x + M.rows[0].y * V.y + M.rows[0].z * V.z;
  result.y = M.rows[1].x * V.x + M.rows[1].y * V.y + M.rows[1].z * V.z;
  result.z = M.rows[2].x * V.x + M.rows[2].y * V.y + M.rows[2].z * V.z;

  return result;
}

inline bool operator==(v3 A, v3 B) {
  bool result = (A.x == B.x && A.y == B.y && A.z == B.z);

  return result;
}

inline bool operator!=(v3 A, v3 B) {
  bool result = (A.x != B.x || A.y != B.y || A.z != B.z);

  return result;
}


// ================= v4 and m4x4 ====================

inline r32 operator*(v4 A, v4 B) {
  r32 result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;

  return result;
}

inline v4 operator*(m4x4 M, v4 V) {
  v4 result = {};

  result.x = M.rows[0].x * V.x + M.rows[0].y * V.y + M.rows[0].z * V.z + M.rows[0].w * V.w;
  result.y = M.rows[1].x * V.x + M.rows[1].y * V.y + M.rows[1].z * V.z + M.rows[1].w * V.w;
  result.z = M.rows[2].x * V.x + M.rows[2].y * V.y + M.rows[2].z * V.z + M.rows[2].w * V.w;
  result.w = M.rows[3].x * V.x + M.rows[3].y * V.y + M.rows[3].z * V.z + M.rows[3].w * V.w;

  return result;
}

// Affine transform
inline v3 operator*(m4x4 M, v3 V) {
  v3 result = {};

  v4 V_ext;
  V_ext.x = V.x;
  V_ext.y = V.y;
  V_ext.z = V.z;
  V_ext.w = 1;

  V_ext = M * V_ext;

  // Homogenize on the fly
  if (V_ext.w != 0.0f && V_ext.w != 1.0f) {
    result = V3(V_ext.x / V_ext.w, V_ext.y / V_ext.w, V_ext.z / V_ext.w);
  } else {
    result = V3(V_ext.x, V_ext.y, V_ext.z);
  }

  return result;
}

inline m4x4 operator*(m4x4 M1, m4x4 M2) {
  m4x4 result;

  for (int i = 0; i < 4 * 4; ++i) {
    int row = i / 4;
    int col = i % 4;
    result.E[i] = M1.rows[row] * M2.column(col);
  }

  return result;
}

// ================= Functions ======================

r32 AngleBetween(v2 A, v2 B);
v2 Transform(m3x3 Matrix, v2 Vector);
v2 Rotate(m3x3 Matrix, v2 Vector, v2 Base);
v3 Rotate(m3x3 Matrix, v3 Vector, v3 Base);
v3 Rotate(m4x4 Matrix, v3 Vector, v3 Base);
v3 Hadamard(v3 A, v3 B);

#endif  // EDITOR_MATH_H
