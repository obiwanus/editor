#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#include <math.h>
#include "ED_base.h"

#ifndef M_PI
#define M_PI (r32)acos(-1.0)
#endif // M_PI

struct v2i {
  int x;
  int y;
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
};

// ==================== Construction ======================

inline v2 V2i(i32 X, i32 Y) {
  v2 result = {(r32)X, (r32)Y};

  return result;
}

inline v2 V2i(u32 X, u32 Y) {
  v2 result = {(r32)X, (r32)Y};

  return result;
}

inline v2 V2(r32 X, r32 Y) {
  v2 result;

  result.x = X;
  result.y = Y;

  return result;
}

inline v3 V3(r32 X, r32 Y, r32 Z) {
  v3 result;

  result.x = X;
  result.y = Y;
  result.z = Z;

  return result;
}

inline v3 V3i(i32 X, i32 Y, i32 Z) {
  v3 result = {(r32)X, (r32)Y, (r32)Z};

  return result;
}

inline v3 V3i(u32 X, u32 Y, u32 Z) {
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

// ================= Functions ======================

r32 AngleBetween(v2 A, v2 B);
v2 Transform(m3x3 Matrix, v2 Vector);
v2 Rotate(m3x3 Matrix, v2 Vector, v2 Base);
v3 Rotate(m3x3 Matrix, v3 Vector, v3 Base);
v3 Hadamard(v3 A, v3 B);

#endif  // EDITOR_MATH_H
