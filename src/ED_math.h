#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#ifndef M_PI
#define M_PI ((r32)acos(-1.0))
#endif  // M_PI

// ================ Misc ======================

inline r32 abs(r32 value) {
  assert(value > 0 || value <= 0);  // catch NaNs
  return value >= 0 ? value : -value;
}

inline i32 round_i32(r32 value) {
  i32 result;
  if (value >= 0) {
    result = (i32)(value + 0.5f);
  } else {
    result = (i32)(value - 0.5f);
  }
  return result;
}

inline r32 floor_r32(r32 value) {
  return (r32)((int)value);
}

int min3(int a, int b, int c) {
  if (a <= b && a <= c) return a;
  if (b <= a && b <= c) return b;
  return c;
}

int max3(int a, int b, int c) {
  if (a >= b && a >= c) return a;
  if (b >= a && b >= c) return b;
  return c;
}

r32 min3(r32 a, r32 b, r32 c) {
  if (a <= b && a <= c) return a;
  if (b <= a && b <= c) return b;
  return c;
}

r32 max3(r32 a, r32 b, r32 c) {
  if (a >= b && a >= c) return a;
  if (b >= a && b >= c) return b;
  return c;
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
  r32 E[3];

  r32 len();
  v3 normalized();
  v3 cross(v3 Vector);
};

// NOTE: thanks @rygorous for an example of how to organise simd operations

// clang-format off

union v4i {
  struct { i32 x, y, z, w; };
  struct { i32 r, g, b, a; };
  i32 E[4];
  __m128i simd;

  v4i() {}
  explicit v4i(i32 x) : simd(_mm_set1_epi32(x)) {}
  explicit v4i(const __m128i &in) : simd(in) {}
  v4i(const v4i &v) : simd(v.simd) {}
  v4i(i32 a, i32 b, i32 c, i32 d) : simd(_mm_setr_epi32(a, b, c, d)) {}

  static v4i zero() { return v4i(_mm_setzero_si128()); }
  static v4i load(const u32 *ptr) { return v4i(_mm_load_si128((__m128i *)ptr)); }
  static v4i loadu(const u32 *ptr) { return v4i(_mm_loadu_si128((__m128i *)ptr)); }
  void store(u32 *ptr) { _mm_store_si128((__m128i *)ptr, simd); }
  void storeu(u32 *ptr) { _mm_storeu_si128((__m128i *)ptr, simd); }

  v4i &operator =(const v4i &v) { simd = v.simd; return *this; }
  v4i &operator+=(const v4i &v) { simd = _mm_add_epi32(simd, v.simd); return *this; }
  v4i &operator-=(const v4i &v) { simd = _mm_sub_epi32(simd, v.simd); return *this; }
  v4i &operator*=(const v4i &v) { simd = _mm_mullo_epi32(simd, v.simd); return *this; }
  v4i &operator|=(const v4i &v) { simd = _mm_or_si128(simd, v.simd); return *this; }
  v4i &operator&=(const v4i &v) { simd = _mm_and_si128(simd, v.simd); return *this; }

  v4i homogenized();
};

inline v4i operator+(const v4i &a, const v4i &b) { return v4i(_mm_add_epi32(a.simd, b.simd)); }
inline v4i operator-(const v4i &a, const v4i &b) { return v4i(_mm_sub_epi32(a.simd, b.simd)); }
inline v4i operator*(const v4i &a, const v4i &b) {
  // TODO: check for SSE4.1 support and use this:
  // return v4i(_mm_mullo_epi32(a.simd, b.simd));
  return v4i(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
inline v4i operator|(const v4i &a, const v4i &b) { return v4i(_mm_or_si128(a.simd, b.simd)); }
inline v4i operator&(const v4i &a, const v4i &b) { return v4i(_mm_and_si128(a.simd, b.simd)); }

inline v4i andnot(const v4i &a, const v4i &b) { return v4i(_mm_andnot_si128(a.simd, b.simd)); }
inline v4i vmin(const v4i &a, const v4i &b) { return v4i(_mm_min_epi32(a.simd, b.simd)); }
inline v4i vmax(const v4i &a, const v4i &b) { return v4i(_mm_max_epi32(a.simd, b.simd)); }

// Functions not operator overloads because the semantics (returns mask)
// are very different from scalar comparison ops.
inline v4i cmplt(const v4i &a, const v4i &b) { return v4i(_mm_cmplt_epi32(a.simd, b.simd)); }
inline v4i cmpgt(const v4i &a, const v4i &b) { return v4i(_mm_cmpgt_epi32(a.simd, b.simd)); }

inline bool is_all_zeros(const v4i &a) { return _mm_testz_si128(a.simd, a.simd) != 0; }
inline bool is_all_negative(const v4i &a) {
  return _mm_testc_si128(_mm_set1_epi32(0x80000000), a.simd) != 0;
}

// can't overload << since parameter to immed. shifts must be a compile-time constant
// and debug builds that don't inline can't deal with this
template<int N> inline v4i shiftl(const v4i &x) { return v4i(_mm_slli_epi32(x.simd, N)); }


union v4 {
  struct { r32 x, y, z, w; };
  struct { r32 r, g, b, a; };
  r32 E[4];
  __m128 simd;

  v4() {}
  explicit v4(r32 x) : simd(_mm_set1_ps(x)) {}
  explicit v4(const __m128 &in) : simd(in) {}
  v4(const v4 &v) : simd(v.simd) {}
  v4(r32 a, r32 b, r32 c, r32 d) : simd(_mm_setr_ps(a, b, c, d)) {}

  static v4 zero() { return v4(_mm_setzero_ps()); }
  static v4 load(const r32 *ptr) { return v4(_mm_load_ps(ptr)); }
  static v4 loadu(const r32 *ptr) { return v4(_mm_loadu_ps(ptr)); }
  void store(r32 *ptr) { _mm_store_ps(ptr, simd); }
  void storeu(r32 *ptr) { _mm_storeu_ps(ptr, simd); }

  v4 &operator =(const v4 &v) { simd = v.simd; return *this; }
  v4 &operator+=(const v4 &v) { simd = _mm_add_ps(simd, v.simd); return *this; }
  v4 &operator-=(const v4 &v) { simd = _mm_sub_ps(simd, v.simd); return *this; }
  v4 &operator*=(const v4 &v) { simd = _mm_mul_ps(simd, v.simd); return *this; }
  v4 &operator/=(const v4 &v) { simd = _mm_div_ps(simd, v.simd); return *this; }

  v4 homogenized();
};

inline v4 operator+(const v4 &a, const v4 &b) { return v4(_mm_add_ps(a.simd, b.simd)); }
inline v4 operator-(const v4 &a, const v4 &b) { return v4(_mm_sub_ps(a.simd, b.simd)); }
inline v4 operator*(const v4 &a, const v4 &b) { return v4(_mm_mul_ps(a.simd, b.simd)); }
inline v4 operator/(const v4 &a, const v4 &b) { return v4(_mm_div_ps(a.simd, b.simd)); }

// Functions not operator overloads because bitwise ops on floats should be explicit
inline v4 v4_or(const v4 &a, const v4 &b) { return v4(_mm_or_ps(a.simd, b.simd)); }
inline v4 v4_and(const v4 &a, const v4 &b) { return v4(_mm_and_ps(a.simd, b.simd)); }
inline v4 v4_and(const v4 &a, const v4 &b, const v4 &c) {
  return v4(_mm_and_ps(a.simd, _mm_and_ps(b.simd, c.simd)));
}
inline v4 v4_and(const v4 &a, const v4 &b, const v4 &c, const v4 &d) {
  return v4(_mm_and_ps(_mm_and_ps(a.simd, b.simd), _mm_and_ps(c.simd, d.simd)));
}

inline v4 vmin(const v4 &a, const v4 &b) { return v4(_mm_min_ps(a.simd, b.simd)); }
inline v4 vmax(const v4 &a, const v4 &b) { return v4(_mm_max_ps(a.simd, b.simd)); }

// Functions not operator overloads because the semantics (returns mask)
// are very different from scalar comparison ops.
inline v4 cmplt(const v4 &a, const v4 &b) { return v4(_mm_cmplt_ps(a.simd, b.simd)); }
inline v4 cmple(const v4 &a, const v4 &b) { return v4(_mm_cmple_ps(a.simd, b.simd)); }
inline v4 cmpgt(const v4 &a, const v4 &b) { return v4(_mm_cmpgt_ps(a.simd, b.simd)); }
inline v4 cmpge(const v4 &a, const v4 &b) { return v4(_mm_cmpge_ps(a.simd, b.simd)); }

inline v4i ftoi(const v4 &v) { return v4i(_mm_cvttps_epi32(v.simd)); }
inline v4i ftoi_round(const v4 &v) { return v4i(_mm_cvtps_epi32(v.simd)); }
inline v4 itof(const v4i &v) { return v4(_mm_cvtepi32_ps(v.simd)); }

inline v4i float2bits(const v4 &v) { return v4i(_mm_castps_si128(v.simd)); }
inline v4 bits2float(const v4i &v) { return v4(_mm_castsi128_ps(v.simd)); }

// Select between a and b based on sign (MSB) of mask
inline v4 select(const v4 &a, const v4 &b, const v4 &mask) {
  return v4(_mm_blendv_ps(a.simd, b.simd, mask.simd));
}
// Select between a and b based on sign (MSB) of mask
inline v4 select(const v4 &a, const v4 &b, const v4i &mask) {
  return v4(_mm_blendv_ps(a.simd, b.simd, _mm_castsi128_ps(mask.simd)));
}

// clang-format on

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

// clang-format off
union m4x4 {
  struct {
    r32 m11, m12, m13, m14;
    r32 m21, m22, m23, m24;
    r32 m31, m32, m33, m34;
    r32 m41, m42, m43, m44;
  };
  struct {
    v4 row0, row1, row2, row3;
  };
  v4 rows[4];
  r32 E[16];

  m4x4() {}
  m4x4(const m4x4 &m) : row0(m.row0), row1(m.row1), row2(m.row2), row3(m.row3) {}
  m4x4(r32 m11, r32 m12, r32 m13, r32 m14,  // C++ I hate you
       r32 m21, r32 m22, r32 m23, r32 m24,
       r32 m31, r32 m32, r32 m33, r32 m34,
       r32 m41, r32 m42, r32 m43, r32 m44) :
       m11(m11), m12(m12), m13(m13), m14(m14),
       m21(m21), m22(m22), m23(m23), m24(m24),
       m31(m31), m32(m32), m33(m33), m34(m34),
       m41(m41), m42(m42), m43(m43), m44(m44) {}

  m4x4 &operator=(const m4x4 &m) {
    row0 = m.row0; row1 = m.row1; row2 = m.row2; row3 = m.row3;
    return *this;
  }

  v4 column(int);
  m4x4 transposed();
};

// clang-format on

namespace Matrix {
m4x4 identity();
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
m4x4 viewport(int, int, int, int, int z_depth = 255);
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

inline v2i V2i(v4 A) {
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

inline v2 V2(v2i A) {
  v2 result;

  result.x = (r32)A.x;
  result.y = (r32)A.y;

  return result;
}

inline v3 V3(v2i A) {
  v3 result;

  result.x = (r32)A.x;
  result.y = (r32)A.y;
  result.z = 0;

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
  v3 result = {X, Y, Z};
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

inline v3 V3(v4 A) {
  v3 result = {A.x, A.y, A.z};
  return result;
}

inline v4 V4_v(v3 A) {
  v4 result = {A.x, A.y, A.z, 0};
  return result;
}

inline v4 V4_p(v3 A) {
  v4 result = {A.x, A.y, A.z, 1};
  return result;
}

inline v4 V4(r32 X, r32 Y, r32 Z, r32 W) {
  v4 result = {X, Y, Z, W};
  return result;
}

inline v4 V4(i32 X, i32 Y, i32 Z, i32 W) {
  v4 result = {(r32)X, (r32)Y, (r32)Z, (r32)W};
  return result;
}

inline v4 V4(u32 X, u32 Y, u32 Z, u32 W) {
  v4 result = {(r32)X, (r32)Y, (r32)Z, (r32)W};
  return result;
}

// ========================== scalar operations ====================

inline r32 square(r32 A) { return A * A; }

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

inline v2i operator*(r32 scalar, v2i A) {
  v2i result = A * scalar;

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

inline v3i operator*(r32 value, v3i A) {
  v3i result = V3i(value * V3(A));

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

// inline r32 operator*(v4 A, v4 B) {
//   r32 result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;

//   return result;
// }

inline v4 operator*(m4x4 &M, v4 V) {
  TIMED_BLOCK();
  v4 result;

  // __m128 brod0 = _mm_set1_ps(V.E[0]);
  // __m128 brod1 = _mm_set1_ps(V.E[1]);
  // __m128 brod2 = _mm_set1_ps(V.E[2]);
  // __m128 brod3 = _mm_set1_ps(V.E[3]);

  // __m128 cols[4];
  // for (int i = 0; i < 4; ++i) {
  //   cols[i] = _mm_set_ps(M.E[12 + i], M.E[8 + i], M.E[4 + i], M.E[0 + i]);
  // }
  // __m128 row = _mm_add_ps(_mm_add_ps(_mm_mul_ps(brod0, cols[0]),
  //                                    _mm_mul_ps(brod1, cols[1])),
  //                         _mm_add_ps(_mm_mul_ps(brod2, cols[2]),
  //                                    _mm_mul_ps(brod3, cols[3])));
  // result.simd = row;

  // Straightforward approach is faster than the SIMD code above,
  // probably because of the preparation code. If we need later we can
  // move it outside of the loop and see if we get any speed up
  result.x = M.rows[0].x * V.x + M.rows[0].y * V.y + M.rows[0].z * V.z +
             M.rows[0].w * V.w;
  result.y = M.rows[1].x * V.x + M.rows[1].y * V.y + M.rows[1].z * V.z +
             M.rows[1].w * V.w;
  result.z = M.rows[2].x * V.x + M.rows[2].y * V.y + M.rows[2].z * V.z +
             M.rows[2].w * V.w;
  result.w = M.rows[3].x * V.x + M.rows[3].y * V.y + M.rows[3].z * V.z +
             M.rows[3].w * V.w;

  return result;
}

// Affine transform
inline v3 operator*(m4x4 &M, v3 V) {
  TIMED_BLOCK();
  v3 result;

  v4 V_ext;
  V_ext.simd = _mm_set_ps(1, V.z, V.y, V.x);

  // TODO: maybe optimize later
  V_ext = M * V_ext;

  // Homogenize on the fly
  if (V_ext.w != 0.0f && V_ext.w != 1.0f) {
    // V_ext.simd = _mm_div_ps(V_ext.simd, _mm_set_ps(V_ext.w));  -slower
    result = V3(V_ext.x / V_ext.w, V_ext.y / V_ext.w, V_ext.z / V_ext.w);
  } else {
    result = V3(V_ext);
  }

  return result;
}

inline m4x4 operator*(m4x4 M1, m4x4 M2) {
  TIMED_BLOCK();
  m4x4 result;
  for (int i = 0; i < 4; i++) {
    __m128 brod1 = _mm_set1_ps(M1.E[4 * i + 0]);
    __m128 brod2 = _mm_set1_ps(M1.E[4 * i + 1]);
    __m128 brod3 = _mm_set1_ps(M1.E[4 * i + 2]);
    __m128 brod4 = _mm_set1_ps(M1.E[4 * i + 3]);
    __m128 row = _mm_add_ps(_mm_add_ps(_mm_mul_ps(brod1, M2.rows[0].simd),
                                       _mm_mul_ps(brod2, M2.rows[1].simd)),
                            _mm_add_ps(_mm_mul_ps(brod3, M2.rows[2].simd),
                                       _mm_mul_ps(brod4, M2.rows[3].simd)));
    result.rows[i].simd = row;
  }
  return result;
}

// ================= Functions ======================

template <typename T>
T min(T val1, T val2) {
  return (val1 < val2) ? val1 : val2;
}

template <typename T>
T max(T val1, T val2) {
  return (val1 > val2) ? val1 : val2;
}

template <typename T>
inline T lerp(T val1, T val2, r32 t) {
  return (T)(val1 + (t * (val2 - val1)));
}

#endif  // EDITOR_MATH_H
