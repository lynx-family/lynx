// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef INCLUDE_SKITY_GEOMETRY_MATRIX_HPP
#define INCLUDE_SKITY_GEOMETRY_MATRIX_HPP

#include <skity/geometry/point.hpp>
#include <skity/macros.hpp>

namespace skity {
namespace clay {

class Rect;

struct SKITY_API Matrix {
 public:
  constexpr static Matrix Translate(float dx, float dy) {
    return Matrix(1.0f, 0.0f, 0.0f, 0.0f,  //
                  0.0f, 1.0f, 0.0f, 0.0f,  //
                  0.0f, 0.0f, 1.0f, 0.0f,  //
                  dx, dy, 0.0f, 1.0f);
  }

  constexpr static Matrix Scale(float sx, float sy) {
    return Matrix(sx, 0.0f, 0.0f, 0.0f,    //
                  0.0f, sy, 0.0f, 0.0f,    //
                  0.0f, 0.0f, 1.0f, 0.0f,  //
                  0.0f, 0.0f, 0.0f, 1.0f);
  }

  constexpr static Matrix Skew(float sx, float sy) {
    return Matrix(1.0f, sy, 0.0f, 0.0f,    //
                  sx, 1.0f, 0.0f, 0.0f,    //
                  0.0f, 0.0f, 1.0f, 0.0f,  //
                  0.0f, 0.0f, 0.0f, 1.0f);
  }

  static Matrix RotateDeg(float deg);

  static Matrix RotateDeg(float deg, Vec2 pt);

  static Matrix RotateDeg(float deg, Vec3 axis);

  static Matrix RotateRad(float rad);

  static Matrix RotateRad(float deg, Vec2 pt);

  ~Matrix() = default;

  constexpr Matrix()
      : Matrix(1.0f, 0.0f, 0.0f, 0.0f,  //
               0.0f, 1.0f, 0.0f, 0.0f,  //
               0.0f, 0.0f, 1.0f, 0.0f,  //
               0.0f, 0.0f, 0.0f, 1.0f) {}

  constexpr Matrix(float s)
      : Matrix(s, 0.0f, 0.0f, 0.0f,  //
               0.0f, s, 0.0f, 0.0f,  //
               0.0f, 0.0f, s, 0.0f,  //
               0.0f, 0.0f, 0.0f, s) {}

  constexpr Matrix(float mxx, float myx, float mzx, float mwx, float mxy,
                   float myy, float mzy, float mwy, float mxz, float myz,
                   float mzz, float mwz, float mxt, float myt, float mzt,
                   float mwt)
      : m{mxx, myx, mzx, mwx,  //
          mxy, myy, mzy, mwy,  //
          mxz, myz, mzz, mwz,  //
          mxt, myt, mzt, mwt} {}

  Matrix(float scale_x, float skew_x, float trans_x, float skew_y,
         float scale_y, float trans_y, float pers_0, float pers_1, float pers_2)
      : Matrix{scale_x, skew_y,  0, pers_0,  //
               skew_x,  scale_y, 0, pers_1,  //
               0,       0,       1, 0,       //
               trans_x, trans_y, 0, pers_2} {}

  Matrix(const Matrix&) = default;

  Matrix& operator=(const Matrix&) = default;

  constexpr bool operator==(const Matrix& other) const {
    return vec[0] == other.vec[0] && vec[1] == other.vec[1] &&
           vec[2] == other.vec[2] && vec[3] == other.vec[3];
  }
  constexpr bool operator!=(const Matrix& other) const {
    return !(*this == other);
  }

  Matrix& Reset() {
    *this = Matrix();
    return *this;
  }

  bool IsIdentity() const;

  bool IsFinite() const;

  static constexpr float kNearZeroFloat = 1.0f / (1 << 12);
  bool IsSimilarity(float tol = kNearZeroFloat) const;

  Matrix& Set9(const float buffer[9]);

  void Get9(float buffer[9]) const;

  Matrix& Set(int row, int column, float value);

  float Get(int row, int column) const;

  constexpr float GetScaleX() const { return e[0][0]; }

  constexpr float GetScaleY() const { return e[1][1]; }

  constexpr float GetSkewX() const { return e[1][0]; }

  constexpr float GetSkewY() const { return e[0][1]; }

  constexpr float GetTranslateX() const { return e[3][0]; }

  constexpr float GetTranslateY() const { return e[3][1]; }

  constexpr float GetPersp0() const { return e[0][3]; }

  constexpr float GetPersp1() const { return e[1][3]; }

  constexpr float GetPersp2() const { return e[3][3]; }

  constexpr void SetScaleX(float s) { e[0][0] = s; }

  constexpr void SetScaleY(float s) { e[1][1] = s; }

  constexpr void SetSkewX(float s) { e[1][0] = s; }

  constexpr void SetSkewY(float s) { e[0][1] = s; }

  constexpr void SetTranslateX(float t) { e[3][0] = t; }

  constexpr void SetTranslateY(float t) { e[3][1] = t; }

  constexpr void SetPersp0(float p) { e[0][3] = p; }

  constexpr void SetPersp1(float p) { e[1][3] = p; }

  constexpr void SetPersp2(float p) { e[3][3] = p; }

  bool Invert(Matrix* inverse) const;

  float Determinant() const;

  void Transpose();

  void MapPoints(Vec2 dst[], const Vec2 src[], int count) const;

  void MapPoints(Point dst[], const Point src[], int count) const;

  bool MapRect(Rect* dst, const Rect& src) const;

  Rect MapRect(const Rect& src) const;

  bool RectStaysRect() const;

  Matrix& PreConcat(const Matrix& other);
  Matrix& PreTranslate(float dx, float dy);
  Matrix& PreScale(float sx, float sy);
  Matrix& PreScale(float sx, float sy, float px, float py);
  Matrix& PreRotate(float degrees);
  Matrix& PreRotate(float degrees, float px, float py);

  Matrix& PostConcat(const Matrix& other);
  Matrix& PostTranslate(float dx, float dy);
  Matrix& PostScale(float sx, float sy);
  Matrix& PostRotate(float degrees);
  Matrix& PostRotate(float degrees, float px, float py);
  Matrix& PostSkew(float kx, float ky);

  bool OnlyScaleAndTranslate() const;

  bool OnlyTranslate() const;

  bool OnlyScale() const;

  bool HasPersp() const;

  friend SKITY_API Matrix operator*(const Matrix& a, const Matrix& b);

  friend SKITY_API Vec4 operator*(const Matrix& m, const Vec4& v);

  constexpr const Vec4& operator[](int i) const { return vec[i]; }

  constexpr Vec4& operator[](int i) { return vec[i]; }

 private:
  bool InvertNonIdentity(Matrix* inverse) const;

  Matrix& SetConcat(const Matrix& left, const Matrix& right);

  union {
    float m[16];
    float e[4][4];
    Vec4 vec[4];
  };
};
}  // namespace clay

using Matrix = clay::Matrix;
}  // namespace skity

#endif  // INCLUDE_SKITY_GEOMETRY_MATRIX_HPP
