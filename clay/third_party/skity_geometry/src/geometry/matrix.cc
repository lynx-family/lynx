// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"
#include "skity/geometry/vector.hpp"
#endif

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/matrix_query.hpp>
#include <memory>
#include <skity/geometry/matrix.hpp>
#include <skity/geometry/rect.hpp>

#include "src/geometry/glm_helper.hpp"
#include "src/geometry/math.hpp"

namespace skity {
namespace clay {

// static
Matrix Matrix::RotateDeg(float deg) {
  return Matrix::RotateRad(FloatDegreesToRadians(deg), {0, 0});
}

// static
Matrix Matrix::RotateDeg(float deg, Vec2 pt) {
  return RotateRad(FloatDegreesToRadians(deg), pt);
}

// static
Matrix Matrix::RotateDeg(float deg, Vec3 axis) {
  return FromGLM(
      glm::rotate(glm::mat4(1.f), FloatDegreesToRadians(deg), ToGLM(axis)));
}

// static
Matrix Matrix::RotateRad(float rad) {
  return Matrix::RotateRad(rad, Vec2{0, 0});
}

// static
Matrix Matrix::RotateRad(float rad, Vec2 pt) {
  glm::mat4 d1(1.f);
  d1[3][0] = -pt.x;
  d1[3][1] = -pt.y;
  glm::mat4 d2(1.f);
  d2[3][0] = pt.x;
  d2[3][1] = pt.y;
  return FromGLM(glm::rotate(d2, rad, glm::vec3(0, 0, 1)) * d1);
}

bool Matrix::IsIdentity() const {
  return glm::isIdentity(ToGLM(*this), glm::epsilon<float>());
}

bool Matrix::IsFinite() const {
  float accum = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      accum *= e[i][j];
    }
  }

  return !FloatIsNan(accum);
}

bool Matrix::IsSimilarity(float tol) const {
  if (HasPersp()) {
    return false;
  }

  float mx = GetScaleX();
  float my = GetScaleY();
  // if no skew, can just compare scale factors
  if (GetSkewX() == 0 && GetSkewY() == 0) {
    return !FloatNearlyZero(mx) && FloatNearlyZero(mx - my);
  }
  float sx = GetSkewX();
  float sy = GetSkewY();

  float perp_dot = mx * my - sx * sy;
  if (FloatNearlyZero(perp_dot, kNearlyZero * kNearlyZero)) {
    return false;
  }

  // upper 2x2 is rotation/reflection + uniform scale if basis vectors
  // are 90 degree rotations of each other
  return (FloatNearlyZero(mx - my, tol) && FloatNearlyZero(sx + sy, tol)) ||
         (FloatNearlyZero(mx + my, tol) && FloatNearlyZero(sx - sy, tol));
}

Matrix& Matrix::Set9(const float buffer[9]) {
  // buffer is in row major, Matrix is in column major
  *this = Matrix{
      buffer[0], buffer[3], 0, buffer[6],  //
      buffer[1], buffer[4], 0, buffer[7],  //
      0,         0,         1, 0,          //
      buffer[2], buffer[5], 0, buffer[8],  //
  };
  return *this;
}

void Matrix::Get9(float buffer[9]) const {
  buffer[0] = GetScaleX();
  buffer[1] = GetSkewX();
  buffer[2] = GetTranslateX();
  buffer[3] = GetSkewY();
  buffer[4] = GetScaleY();
  buffer[5] = GetTranslateY();
  buffer[6] = GetPersp0();
  buffer[7] = GetPersp1();
  buffer[8] = GetPersp2();
}

Matrix& Matrix::Set(int row, int column, float value) {
  if (row < 0 || row > 3 || column < 0 || column > 3) {
    return *this;
  }
  Matrix& m = *this;
  m[column][row] = value;
  return *this;
}

float Matrix::Get(int row, int column) const {
  if (row < 0 || row > 3 || column < 0 || column > 3) {
    return 0.f;
  }
  const Matrix& m = *this;
  return m[column][row];
}

bool Matrix::Invert(Matrix* inverse) const {
  if (IsIdentity()) {
    if (inverse != nullptr) {
      *inverse = Matrix(1.0);
    }
    return true;
  }
  return this->InvertNonIdentity(inverse);
}

float Matrix::Determinant() const {
  if (IsIdentity()) {
    return 1.f;
  }
  const Matrix& m = *this;
  if (OnlyScaleAndTranslate()) {
    return m[0][0] * m[1][1] * m[2][2] * m[3][3];
  }
  return glm::determinant(ToGLM(*this));
}

void Matrix::Transpose() { *this = FromGLM(glm::transpose(ToGLM(*this))); }

bool Matrix::OnlyScaleAndTranslate() const {
  auto& m = *this;
  // No z
  if (m[0][2] != 0 || m[1][2] != 0 || m[2][2] != 1 || m[3][2] != 0 ||
      m[2][0] != 0 || m[2][1] != 0) {
    return false;
  }
  // no persp
  if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) {
    return false;
  }
  // no skew
  if (m[0][1] != 0 || m[1][0] != 0) {
    return false;
  }
  return true;
}

bool Matrix::OnlyTranslate() const {
  auto& m = *this;
  // No Scale
  if (m[0][0] != 1.f || m[1][1] != 1.f) {
    return false;
  }
  // No z
  if (m[0][2] != 0 || m[1][2] != 0 || m[2][2] != 1 || m[3][2] != 0 ||
      m[2][0] != 0 || m[2][1] != 0) {
    return false;
  }
  // no persp
  if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) {
    return false;
  }
  // no skew
  if (m[0][1] != 0 || m[1][0] != 0) {
    return false;
  }
  return true;
}

bool Matrix::OnlyScale() const {
  auto& m = *this;
  // No translate
  if (m[3][0] != 0 || m[3][1] != 0) {
    return false;
  }
  // No z
  if (m[0][2] != 0 || m[1][2] != 0 || m[2][2] != 1 || m[3][2] != 0 ||
      m[2][0] != 0 || m[2][1] != 0) {
    return false;
  }
  // no persp
  if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) {
    return false;
  }
  // no skew
  if (m[0][1] != 0 || m[1][0] != 0) {
    return false;
  }
  return true;
}

bool Matrix::HasPersp() const {
  const Matrix& m = *this;
  if (m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) {
    return true;
  }
  return false;
}

bool Matrix::InvertNonIdentity(Matrix* inverse) const {
  Matrix temp_inverse;
  Matrix* p_inverse =
      (inverse == nullptr || inverse == this) ? &temp_inverse : inverse;

  // short path
  if (OnlyScaleAndTranslate()) {
    static_cast<Matrix*>(p_inverse)->Reset();
    if (GetScaleX() != 1.f || GetScaleY() != 1.f) {
      float inverse_x = SkityIEEEFloatDivided(1.f, GetScaleX());
      float inverse_y = SkityIEEEFloatDivided(1.f, GetScaleY());
      if (FloatInfinity == inverse_x || FloatInfinity == inverse_y) {
        return false;
      }
      p_inverse->SetScaleX(inverse_x);
      p_inverse->SetScaleY(inverse_y);
      p_inverse->SetTranslateX(-GetTranslateX() * inverse_x);
      p_inverse->SetTranslateY(-GetTranslateY() * inverse_y);
    } else {
      p_inverse->SetTranslateX(-GetTranslateX());
      p_inverse->SetTranslateY(-GetTranslateY());
    }
    if (inverse == this) {
      *inverse = *p_inverse;
    }
    return true;
  }

  if (FloatNearlyZero(glm::determinant(ToGLM(*this)))) {
    return false;
  }

  *p_inverse = FromGLM(glm::inverse(ToGLM(*this)));
  if (inverse == this) {
    *inverse = *p_inverse;
  }
  return true;
}

void Matrix::MapPoints(Vec2 dst[], const Vec2 src[], int count) const {
  if (dst == nullptr || src == nullptr || count <= 0) {
    return;
  }
  for (int i = 0; i < count; ++i) {
    auto v = Vec4(src[i].x, src[i].y, 0.f, 1.f);
    auto r = (*this) * v;
    float w = r.w;
    if (w != 0) {
      w = 1.f / w;
    }
    dst[i].x = r.x * w;
    dst[i].y = r.y * w;
  }
}

void Matrix::MapPoints(Point dst[], const Point src[], int count) const {
  if (dst == nullptr || src == nullptr || count <= 0) {
    return;
  }
  for (int i = 0; i < count; ++i) {
    dst[i] = (*this) * src[i];
  }
}

bool Matrix::MapRect(Rect* dst, const Rect& src) const {
  if (dst == nullptr) {
    return false;
  }

  Vec4 src_quad[4] = {{src.Left(), src.Top(), 0.f, 1.f},
                      {src.Right(), src.Top(), 0.f, 1.f},
                      {src.Right(), src.Bottom(), 0.f, 1.f},
                      {src.Left(), src.Bottom(), 0.f, 1.f}};
  Vec4 dst_quad[4];
  for (size_t i = 0; i < 4; ++i) {
    dst_quad[i] = (*this) * src_quad[i];
    float w = dst_quad[i].w;
    if (w != 0) {
      w = 1.0f / w;
    }
    dst_quad[i] = dst_quad[i] * w;
  }
  float left = dst_quad[0].x;
  float right = dst_quad[0].x;
  float top = dst_quad[0].y;
  float bottom = dst_quad[0].y;
  for (size_t i = 1; i < 4; i++) {
    left = std::min(dst_quad[i].x, left);
    right = std::max(dst_quad[i].x, right);
    top = std::min(dst_quad[i].y, top);
    bottom = std::max(dst_quad[i].y, bottom);
  }
  dst->SetLTRB(left, top, right, bottom);

  return RectStaysRect();
}

Rect Matrix::MapRect(const Rect& src) const {
  Rect dst;
  MapRect(&dst, src);
  return dst;
}

bool Matrix::RectStaysRect() const {
  auto& m = *this;
  if (m[0][2] != 0 || m[1][2] != 0 || m[2][2] != 1 || m[3][2] != 0 ||
      m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1) {
    return false;
  }

  if (m[0][1] != 0 || m[1][0] != 0) {
    return m[0][0] == 0 && m[1][1] == 0 && m[1][0] != 0 && m[0][1] != 0;
  } else {
    return m[0][0] != 0 && m[1][1] != 0;
  }
}

Matrix& Matrix::PreConcat(const Matrix& other) {
  if (!other.IsIdentity()) {
    SetConcat(*this, other);
  }
  return *this;
}

Matrix& Matrix::PostConcat(const Matrix& other) {
  if (!other.IsIdentity()) {
    SetConcat(other, *this);
  }
  return *this;
}

Matrix& Matrix::PreTranslate(float dx, float dy) {
  Matrix m = Matrix::Translate(dx, dy);
  SetConcat(*this, m);
  return *this;
}

Matrix& Matrix::PostTranslate(float dx, float dy) {
  Matrix m = Matrix::Translate(dx, dy);
  SetConcat(m, *this);
  return *this;
}

Matrix& Matrix::PreScale(float sx, float sy) {
  Matrix m = Matrix::Scale(sx, sy);
  SetConcat(*this, m);
  return *this;
}

Matrix& Matrix::PostScale(float sx, float sy) {
  Matrix m = Matrix::Scale(sx, sy);
  SetConcat(m, *this);
  return *this;
}

Matrix& Matrix::PreScale(float sx, float sy, float px, float py) {
  Matrix d1 = Matrix::Translate(-px, -py);
  Matrix s = Matrix::Scale(sx, sy);
  Matrix d2 = Matrix::Translate(px, py);
  auto result = *this * d2 * s * d1;
  *this = result;
  return *this;
}

Matrix& Matrix::PreRotate(float degrees) {
  return this->PreRotate(degrees, 0, 0);
}

Matrix& Matrix::PostRotate(float degrees) {
  return this->PostRotate(degrees, 0, 0);
}

Matrix& Matrix::PreRotate(float degrees, float px, float py) {
  auto r = Matrix::RotateDeg(degrees, Vec2(px, py));
  return this->PreConcat(r);
}

Matrix& Matrix::PostRotate(float degrees, float px, float py) {
  auto r = Matrix::RotateDeg(degrees, Vec2(px, py));
  return this->PostConcat(r);
}

Matrix& Matrix::PostSkew(float kx, float ky) {
  Matrix m = Matrix::Skew(kx, ky);
  return this->PostConcat(m);
}

// Notice: we ignore element related z.
Matrix& Matrix::SetConcat(const Matrix& left, const Matrix& right) {
  if (left.IsIdentity()) {
    *this = right;
  } else if (right.IsIdentity()) {
    *this = left;
  } else if (left.OnlyScaleAndTranslate() && right.OnlyScaleAndTranslate()) {
    float scale_x = left.GetScaleX() * right.GetScaleX();
    float scale_y = left.GetScaleY() * right.GetScaleY();
    float tranlate_x =
        left.GetScaleX() * right.GetTranslateX() + left.GetTranslateX();
    float tranlate_y =
        left.GetScaleY() * right.GetTranslateY() + left.GetTranslateY();
    this->SetScaleX(scale_x);
    this->SetScaleY(scale_y);
    this->SetTranslateX(tranlate_x);
    this->SetTranslateY(tranlate_y);
  } else {
    *this = left * right;
  }
  return *this;
}

SKITY_API Matrix operator*(const Matrix& a, const Matrix& b) {
  if (a.IsIdentity()) {
    return b;
  }
  if (b.IsIdentity()) {
    return a;
  }
  return FromGLM(ToGLM(a) * ToGLM(b));
}

Vec4 operator*(const Matrix& m, const Vec4& v) {
  glm::vec4 r = ToGLM(m) * ToGLM(v);
  return Vec4{r.x, r.y, r.z, r.w};
}

}  // namespace clay
}  // namespace skity
