// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#endif

#if defined(__clang__) || defined(__GNUC__)
#define NO_SANITIZE(A) __attribute__((no_sanitize(A)))
#else
#define NO_SANITIZE(A)
#endif

#include "core/animation/transforms/matrix44.h"

#include <cmath>
#include <cstring>
#include <string>

#include "base/include/float_comparison.h"

namespace lynx {
namespace transforms {
static inline constexpr double DegToRad(double degrees) {
  return degrees * M_PI / 180.0;
}

NO_SANITIZE("float-divide-by-zero")
static constexpr double IEEEDoubleDivide(double numer, double denom) {
  return numer / denom;
}

static inline bool IsFinite(const float array[], int count) {
  float x = array[0];
  float prod = x - x;
  for (int i = 1; i < count; ++i) {
    prod *= array[i];
  }

  return prod == prod;
}

void Matrix44::recomputeTypeMask() {
  if (0 != perspX() || 0 != perspY() || 0 != perspZ() || 1 != fMat[3][3]) {
    fTypeMask =
        kTranslate_Mask | kScale_Mask | kAffine_Mask | kPerspective_Mask;
    return;
  }

  TypeMask mask = kIdentity_Mask;
  if (0 != transX() || 0 != transY() || 0 != transZ()) {
    mask |= kTranslate_Mask;
  }

  if (1 != scaleX() || 1 != scaleY() || 1 != scaleZ()) {
    mask |= kScale_Mask;
  }

  if (0 != fMat[1][0] || 0 != fMat[0][1] || 0 != fMat[0][2] ||
      0 != fMat[2][0] || 0 != fMat[1][2] || 0 != fMat[2][1]) {
    mask |= kAffine_Mask;
  }
  fTypeMask = mask;
}

void Matrix44::setIdentity() {
  fMat[0][0] = 1;
  fMat[0][1] = 0;
  fMat[0][2] = 0;
  fMat[0][3] = 0;
  fMat[1][0] = 0;
  fMat[1][1] = 1;
  fMat[1][2] = 0;
  fMat[1][3] = 0;
  fMat[2][0] = 0;
  fMat[2][1] = 0;
  fMat[2][2] = 1;
  fMat[2][3] = 0;
  fMat[3][0] = 0;
  fMat[3][1] = 0;
  fMat[3][2] = 0;
  fMat[3][3] = 1;
  this->setTypeMask(kIdentity_Mask);
}

Matrix44& Matrix44::preTranslate(float dx, float dy, float dz) {
  if (!dx && !dy && !dz) {
    return *this;
  }

  for (int i = 0; i < 4; ++i) {
    fMat[3][i] =
        fMat[0][i] * dx + fMat[1][i] * dy + fMat[2][i] * dz + fMat[3][i];
  }
  this->recomputeTypeMask();
  return *this;
}

Matrix44& Matrix44::preScale(float sx, float sy, float sz) {
  if (1 == sx && 1 == sy && 1 == sz) {
    return *this;
  }

  // The implementation matrix * pureScale can be shortcut
  // by knowing that pureScale components effectively scale
  // the columns of the original matrix.
  for (int i = 0; i < 4; i++) {
    fMat[0][i] *= sx;
    fMat[1][i] *= sy;
    fMat[2][i] *= sz;
  }
  this->recomputeTypeMask();
  return *this;
}

void Matrix44::setRotateAboutXAxis(float deg) {
  double sin_theta = std::sin(DegToRad(deg));
  double cos_theta = std::cos(DegToRad(deg));
  fMat[0][0] = 1;
  fMat[0][1] = 0;
  fMat[0][2] = 0;
  fMat[0][3] = 0;
  fMat[1][0] = 0;
  fMat[1][1] = cos_theta;
  fMat[1][2] = sin_theta;
  fMat[1][3] = 0;
  fMat[2][0] = 0;
  fMat[2][1] = -sin_theta;
  fMat[2][2] = cos_theta;
  fMat[2][3] = 0;
  fMat[3][0] = 0;
  fMat[3][1] = 0;
  fMat[3][2] = 0;
  fMat[3][3] = 1;

  this->recomputeTypeMask();
}

void Matrix44::setRotateAboutYAxis(float deg) {
  double sin_theta = std::sin(DegToRad(deg));
  double cos_theta = std::cos(DegToRad(deg));
  fMat[0][0] = cos_theta;
  fMat[0][1] = 0;
  fMat[0][2] = -sin_theta;
  fMat[0][3] = 0;
  fMat[1][0] = 0;
  fMat[1][1] = 1;
  fMat[1][2] = 0;
  fMat[1][3] = 0;
  fMat[2][0] = sin_theta;
  fMat[2][1] = 0;
  fMat[2][2] = cos_theta;
  fMat[2][3] = 0;
  fMat[3][0] = 0;
  fMat[3][1] = 0;
  fMat[3][2] = 0;
  fMat[3][3] = 1;

  this->recomputeTypeMask();
}

void Matrix44::setRotateAboutZAxis(float deg) {
  double sin_theta = std::sin(DegToRad(deg));
  double cos_theta = std::cos(DegToRad(deg));
  fMat[0][0] = cos_theta;
  fMat[0][1] = sin_theta;
  fMat[0][2] = 0;
  fMat[0][3] = 0;
  fMat[1][0] = -sin_theta;
  fMat[1][1] = cos_theta;
  fMat[1][2] = 0;
  fMat[1][3] = 0;
  fMat[2][0] = 0;
  fMat[2][1] = 0;
  fMat[2][2] = 1;
  fMat[2][3] = 0;
  fMat[3][0] = 0;
  fMat[3][1] = 0;
  fMat[3][2] = 0;
  fMat[3][3] = 1;

  this->recomputeTypeMask();
}

void Matrix44::Skew(float angle_x, float angle_y) {
  if (isIdentity()) {
    setRC(0, 1, std::tan(DegToRad(angle_x)));
    setRC(1, 0, std::tan(DegToRad(angle_y)));
  } else {
    Matrix44 skew;
    skew.setRC(0, 1, std::tan(DegToRad(angle_x)));
    skew.setRC(1, 0, std::tan(DegToRad(angle_y)));
    preConcat(skew);
  }
}

void Matrix44::Matrix(const std::array<float, 16>& matrix_raw_value) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      fMat[i][j] = matrix_raw_value.at(i * 4 + j);
    }
  }
  this->recomputeTypeMask();
}

static bool IsBitsOnly(int value, int mask) { return 0 == (value & ~mask); }

void Matrix44::setConcat(const Matrix44& a, const Matrix44& b) {
  const Matrix44::TypeMask a_mask = a.getType();
  const Matrix44::TypeMask b_mask = b.getType();

  if (kIdentity_Mask == a_mask) {
    *this = b;
    return;
  }
  if (kIdentity_Mask == b_mask) {
    *this = a;
    return;
  }

  bool useStorage = (this == &a || this == &b);
  float storage[16];
  float* result = useStorage ? storage : &fMat[0][0];

  // Both matrices are at most scale+translate
  if (IsBitsOnly(a_mask | b_mask, kScale_Mask | kTranslate_Mask)) {
    result[0] = a.fMat[0][0] * b.fMat[0][0];
    result[1] = result[2] = result[3] = result[4] = 0;
    result[5] = a.fMat[1][1] * b.fMat[1][1];
    result[6] = result[7] = result[8] = result[9] = 0;
    result[10] = a.fMat[2][2] * b.fMat[2][2];
    result[11] = 0;
    result[12] = a.fMat[0][0] * b.fMat[3][0] + a.fMat[3][0];
    result[13] = a.fMat[1][1] * b.fMat[3][1] + a.fMat[3][1];
    result[14] = a.fMat[2][2] * b.fMat[3][2] + a.fMat[3][2];
    result[15] = 1;
  } else {
    for (const auto& row : b.fMat) {
      for (int i = 0; i < 4; i++) {
        double value = 0;
        for (int k = 0; k < 4; k++) {
          value += double(a.fMat[k][i]) * row[k];
        }
        *result++ = float(value);
      }
    }
  }

  if (useStorage) {
    std::memcpy(fMat, storage, sizeof(storage));
  }
  this->recomputeTypeMask();
}

/** We always perform the calculation in doubles, to avoid prematurely losing
    precision along the way. This relies on the compiler automatically
    promoting our float values to double (if needed).
 */
double Matrix44::determinant() const {
  if (this->isIdentity()) {
    return 1;
  }
  if (this->isScaleTranslate()) {
    return fMat[0][0] * fMat[1][1] * fMat[2][2] * fMat[3][3];
  }

  double a00 = fMat[0][0];
  double a01 = fMat[0][1];
  double a02 = fMat[0][2];
  double a03 = fMat[0][3];
  double a10 = fMat[1][0];
  double a11 = fMat[1][1];
  double a12 = fMat[1][2];
  double a13 = fMat[1][3];
  double a20 = fMat[2][0];
  double a21 = fMat[2][1];
  double a22 = fMat[2][2];
  double a23 = fMat[2][3];
  double a30 = fMat[3][0];
  double a31 = fMat[3][1];
  double a32 = fMat[3][2];
  double a33 = fMat[3][3];

  double b00 = a00 * a11 - a01 * a10;
  double b01 = a00 * a12 - a02 * a10;
  double b02 = a00 * a13 - a03 * a10;
  double b03 = a01 * a12 - a02 * a11;
  double b04 = a01 * a13 - a03 * a11;
  double b05 = a02 * a13 - a03 * a12;
  double b06 = a20 * a31 - a21 * a30;
  double b07 = a20 * a32 - a22 * a30;
  double b08 = a20 * a33 - a23 * a30;
  double b09 = a21 * a32 - a22 * a31;
  double b10 = a21 * a33 - a23 * a31;
  double b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  return b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
}

void Matrix44::mapPoint(float dst_point[2], const float src_point[2]) const {
  if (!dst_point || !src_point) {
    return;
  }

  float src_x = src_point[0], src_y = src_point[1];
  // Calculate the normalization parameter.
  float w = fMat[0][3] * src_x + fMat[1][3] * src_y + fMat[3][3];
  if (base::FloatsEqual(w, 0.f)) {
    return;
  }

  // matrix is column-major.
  float dst_x = fMat[0][0] * src_x + fMat[1][0] * src_y + fMat[3][0];
  float dst_y = fMat[0][1] * src_x + fMat[1][1] * src_y + fMat[3][1];
  dst_point[0] = dst_x / w;
  dst_point[1] = dst_y / w;
}

bool Matrix44::invert(Matrix44* inverse) const {
  if (!inverse) {
    return false;
  }

  float tmp[16];
  if (invert4x4Matrix(reinterpret_cast<const float*>(this->fMat), tmp) ==
      0.0f) {
    return false;
  }
  memcpy(reinterpret_cast<float*>(inverse->fMat), tmp, sizeof(tmp));
  return true;
}

double Matrix44::invert4x4Matrix(const float in_matrix[16],
                                 float out_matrix[16]) const {
  double a00 = in_matrix[0];
  double a01 = in_matrix[1];
  double a02 = in_matrix[2];
  double a03 = in_matrix[3];
  double a10 = in_matrix[4];
  double a11 = in_matrix[5];
  double a12 = in_matrix[6];
  double a13 = in_matrix[7];
  double a20 = in_matrix[8];
  double a21 = in_matrix[9];
  double a22 = in_matrix[10];
  double a23 = in_matrix[11];
  double a30 = in_matrix[12];
  double a31 = in_matrix[13];
  double a32 = in_matrix[14];
  double a33 = in_matrix[15];

  double b00 = a00 * a11 - a01 * a10;
  double b01 = a00 * a12 - a02 * a10;
  double b02 = a00 * a13 - a03 * a10;
  double b03 = a01 * a12 - a02 * a11;
  double b04 = a01 * a13 - a03 * a11;
  double b05 = a02 * a13 - a03 * a12;
  double b06 = a20 * a31 - a21 * a30;
  double b07 = a20 * a32 - a22 * a30;
  double b08 = a20 * a33 - a23 * a30;
  double b09 = a21 * a32 - a22 * a31;
  double b10 = a21 * a33 - a23 * a31;
  double b11 = a22 * a33 - a23 * a32;

  // Calculate the determinant
  double determinant =
      b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
  if (out_matrix) {
    double invdet = IEEEDoubleDivide(1.0, determinant);
    b00 *= invdet;
    b01 *= invdet;
    b02 *= invdet;
    b03 *= invdet;
    b04 *= invdet;
    b05 *= invdet;
    b06 *= invdet;
    b07 *= invdet;
    b08 *= invdet;
    b09 *= invdet;
    b10 *= invdet;
    b11 *= invdet;

    out_matrix[0] = a11 * b11 - a12 * b10 + a13 * b09;
    out_matrix[1] = a02 * b10 - a01 * b11 - a03 * b09;
    out_matrix[2] = a31 * b05 - a32 * b04 + a33 * b03;
    out_matrix[3] = a22 * b04 - a21 * b05 - a23 * b03;
    out_matrix[4] = a12 * b08 - a10 * b11 - a13 * b07;
    out_matrix[5] = a00 * b11 - a02 * b08 + a03 * b07;
    out_matrix[6] = a32 * b02 - a30 * b05 - a33 * b01;
    out_matrix[7] = a20 * b05 - a22 * b02 + a23 * b01;
    out_matrix[8] = a10 * b10 - a11 * b08 + a13 * b06;
    out_matrix[9] = a01 * b08 - a00 * b10 - a03 * b06;
    out_matrix[10] = a30 * b04 - a31 * b02 + a33 * b00;
    out_matrix[11] = a21 * b02 - a20 * b04 - a23 * b00;
    out_matrix[12] = a11 * b07 - a10 * b09 - a12 * b06;
    out_matrix[13] = a00 * b09 - a01 * b07 + a02 * b06;
    out_matrix[14] = a31 * b01 - a30 * b03 - a32 * b00;
    out_matrix[15] = a20 * b03 - a21 * b01 + a22 * b00;

    // If 1/det overflows to infinity (i.e. det is denormalized) or any of the
    // inverted matrix values is non-finite, return zero to indicate a
    // non-invertible matrix.
    if (!IsFinite(out_matrix, 16)) {
      determinant = 0.0f;
    }
  }
  return determinant;
}

}  // namespace transforms
}  // namespace lynx
