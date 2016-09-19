#include "ED_math.h"

r32 v2::len() {
  return (r32)sqrt(x * x + y * y);
}

r32 v3::len() {
  return (r32)sqrt(x * x + y * y + z * z);
}

v2 v2::normalized() {
  v2 result = *this / this->len();
  return result;
}

v3 v3::normalized() {
  v3 result = *this / this->len();
  return result;
}

r32 AngleBetween(v2 A, v2 B) {
  r32 cosine = (A * B) / (A.len() * B.len());
  r32 result = (r32)acos(cosine);
  if (A.y <= B.y) {
    result = 2* M_PI - result;
  }
  return result;
}

v2 Transform(m3x3 Matrix, v2 Vector) {
  // Note the vector is v2
  v2 result = {};
  v3 v = {Vector.x, Vector.y, 1.0f};

  v = Matrix * v;
  result = {v.x, v.y};

  return result;
}

v2 Rotate(m3x3 Matrix, v2 Vector, v2 Base) {
  v2 result = Vector - Base;

  result = Transform(Matrix, result);
  result += Base;

  return result;
}

v3 Rotate(m3x3 Matrix, v3 Vector, v3 Base) {
  v3 result = Vector - Base;

  result = Matrix * result;
  result += Base;

  return result;
}
