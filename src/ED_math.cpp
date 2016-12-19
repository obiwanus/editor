#include "ED_math.h"

r32 v2i::len() { return (r32)sqrt(x * x + y * y); }

r32 v2::len() { return (r32)sqrt(x * x + y * y); }

r32 v3::len() { return (r32)sqrt(x * x + y * y + z * z); }

v2 v2::normalized() {
  v2 result = *this / this->len();
  return result;
}

v3 v3::normalized() {
  v3 result = *this / this->len();
  return result;
}

v3 v3::cross(v3 Vector) {
  v3 result;

  result.x = y * Vector.z - z * Vector.y;
  result.y = z * Vector.x - x * Vector.z;
  result.z = x * Vector.y - y * Vector.x;

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

v4 m4x4::column(int number) {
  v4 result;

  for (int row = 0; row < 4; ++row) {
    result.E[row] = this->E[4 * row + number];
  }

  return result;
}

r32 AngleBetween(v2 A, v2 B) {
  r32 cosine = (A * B) / (A.len() * B.len());
  r32 result = (r32)acos(cosine);
  if (A.y <= B.y) {
    result = 2 * M_PI - result;
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

v3 Rotate(m4x4 Matrix, v3 Vector, v3 Base) {
  v3 result = Vector - Base;
  m3x3 TruncatedMatrix = {
    Matrix.E[0], Matrix.E[1], Matrix.E[2],
    Matrix.E[4], Matrix.E[5], Matrix.E[6],
    Matrix.E[8], Matrix.E[9], Matrix.E[10],
  };

  result = TruncatedMatrix * result;
  result += Base;
  return result;
}

v3 Hadamard(v3 A, v3 B) {
  v3 result;

  result.x = A.x * B.x;
  result.y = A.y * B.y;
  result.z = A.z * B.z;

  return result;
}
