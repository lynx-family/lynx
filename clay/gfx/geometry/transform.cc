// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/transform.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point3d.h"
#include "clay/gfx/geometry/float_vector3d.h"
#include "clay/gfx/geometry/math_util.h"
#include "clay/gfx/geometry/quaternion.h"
#include "clay/gfx/geometry/transform_util.h"

namespace clay {

namespace {

const float kEpsilon = std::numeric_limits<float>::epsilon();

float TanDegrees(double degrees) { return std::tan(DegToRad(degrees)); }

inline bool ApproximatelyZero(float x, float tolerance) {
  return std::abs(x) <= tolerance;
}

inline bool ApproximatelyOne(float x, float tolerance) {
  return std::abs(x - 1) <= tolerance;
}

// Referenced from skity::Quaternion::EulerToMatrix
skity::Matrix EulerToMatrix(float alpha, float beta, float gamma) {
  const auto cos_alpha = std::cos(alpha);
  const auto sin_alpha = std::sin(alpha);
  const auto cos_beta = std::cos(beta);
  const auto sin_beta = std::sin(beta);
  const auto cos_gamma = std::cos(gamma);
  const auto sin_gamma = std::sin(gamma);

  return skity::Matrix(cos_beta * cos_gamma,
                       sin_alpha * sin_beta * cos_gamma + cos_alpha * sin_gamma,
                       sin_alpha * sin_gamma - cos_alpha * sin_beta * cos_gamma,
                       0, -cos_beta * sin_gamma,
                       cos_alpha * cos_gamma - sin_alpha * sin_beta * sin_gamma,
                       cos_alpha * sin_beta * sin_gamma + sin_alpha * cos_gamma,
                       0, sin_beta, -sin_alpha * cos_beta, cos_alpha * cos_beta,
                       0, 0, 0, 0, 1);
}

}  // namespace

Transform::Transform(float col1row1, float col2row1, float col3row1,
                     float col4row1, float col1row2, float col2row2,
                     float col3row2, float col4row2, float col1row3,
                     float col2row3, float col3row3, float col4row3,
                     float col1row4, float col2row4, float col3row4,
                     float col4row4) {
  matrix_ =
      skity::Matrix(col1row1, col1row2, col1row3, col1row4, col2row1, col2row2,
                    col2row3, col2row4, col3row1, col3row2, col3row3, col3row4,
                    col4row1, col4row2, col4row3, col4row4);
}

Transform::Transform(float col1row1, float col2row1, float col1row2,
                     float col2row2, float x_translation, float y_translation) {
  matrix_.Set(0, 0, col1row1);
  matrix_.Set(1, 0, col1row2);
  matrix_.Set(0, 1, col2row1);
  matrix_.Set(1, 1, col2row2);
  matrix_.Set(0, 3, x_translation);
  matrix_.Set(1, 3, y_translation);
}

Transform::Transform(const Quaternion& q) {
  float x = q.x();
  float y = q.y();
  float z = q.z();
  float w = q.w();

  // Implicitly calls matrix.Reset()
  const float buffer[9] = {1.f - 2.f * (y * y + z * z), 2.f * (x * y + z * w),
                           2.f * (x * z - y * w),       2.f * (x * y - z * w),
                           1.f - 2.f * (x * x + z * z), 2.f * (y * z + x * w),
                           2.f * (x * z + y * w),       2.f * (y * z - x * w),
                           1.f - 2.f * (x * x + y * y)};
  matrix_.Set9(buffer);
}

void Transform::RotateAboutXAxis(double degrees) {
  double radians = DegToRad(degrees);
  float cosTheta = std::cos(radians);
  float sinTheta = std::sin(radians);
  const float buffer[9] = {1,        0, 0,         0,       cosTheta,
                           sinTheta, 0, -sinTheta, cosTheta};
  if (matrix_.IsIdentity()) {
    matrix_.Set9(buffer);
  } else {
    skity::Matrix rot;
    rot.Set9(buffer);
    matrix_.PreConcat(rot);
  }
}

void Transform::RotateAboutYAxis(double degrees) {
  double radians = DegToRad(degrees);
  float cosTheta = std::cos(radians);
  float sinTheta = std::sin(radians);
  const float buffer[9] = {cosTheta, 0,        -sinTheta, 0,       1,
                           0,        sinTheta, 0,         cosTheta};
  if (matrix_.IsIdentity()) {
    // Note carefully the placement of the -sinTheta for rotation about
    // y-axis is different than rotation about x-axis or z-axis.
    matrix_.Set9(buffer);
  } else {
    skity::Matrix rot;
    rot.Set9(buffer);
    matrix_.PreConcat(rot);
  }
}

void Transform::RotateAboutZAxis(double degrees) {
  double radians = DegToRad(degrees);
  float cosTheta = std::cos(radians);
  float sinTheta = std::sin(radians);
  const float buffer[9] = {cosTheta, sinTheta, 0, -sinTheta, cosTheta,
                           0,        0,        0, 1};
  if (matrix_.IsIdentity()) {
    matrix_.Set9(buffer);
  } else {
    skity::Matrix rot;
    rot.Set9(buffer);
    matrix_.PreConcat(rot);
  }
}

void Transform::RotateAbout(const FloatVector3d& axis, double degrees) {
  float x = axis.x();
  float y = axis.y();
  float z = axis.z();
  double radians = DegToRad(degrees);
  // We always use separated x, y, z for the rotate transform animation.
  // See TransformOperations::AppendRotate.
  if (z == 1 && y == 0 && x == 0) {
    const auto matrix = EulerToMatrix(0, 0, radians);
    matrix_.PreConcat(matrix);
  } else if (z == 0 && y == 1 && x == 0) {
    const auto matrix = EulerToMatrix(0, radians, 0);
    matrix_.PreConcat(matrix);
  } else if (z == 0 && y == 0 && x == 1) {
    const auto matrix = EulerToMatrix(radians, 0, 0);
    matrix_.PreConcat(matrix);
  } else {
    FML_UNREACHABLE();
  }
}

void Transform::Scale(float x, float y) { matrix_.PreScale(x, y); }

void Transform::PostScale(float x, float y) { matrix_.PostScale(x, y); }

void Transform::Scale3d(float x, float y, float z) {
  if (z != 1.f) {
    FML_LOG(WARNING)
        << "Skity has ignored the element related z when Scale3D. Z = " << z;
  }
  matrix_.PreScale(x, y);
}

void Transform::Translate(const FloatVector2d& offset) {
  Translate(offset.x(), offset.y());
}

void Transform::Translate(float x, float y) { matrix_.PreTranslate(x, y); }

void Transform::PostTranslate(const FloatVector2d& offset) {
  PostTranslate(offset.x(), offset.y());
}

void Transform::PostTranslate(float x, float y) { matrix_.PostTranslate(x, y); }

void Transform::Translate3d(const FloatVector3d& offset) {
  Translate3d(offset.x(), offset.y(), offset.z());
}

void Transform::Translate3d(float x, float y, float z) {
  FML_LOG(ERROR) << "Skity has ignored the element related z when Translate3d.";
  matrix_.PreTranslate(x, y);
}

void Transform::Skew(double angle_x, double angle_y) {
  if (matrix_.IsIdentity()) {
    matrix_.Set(0, 1, TanDegrees(angle_x));
    matrix_.Set(1, 0, TanDegrees(angle_y));
  } else {
    skity::Matrix skew;
    skew.Set(0, 1, TanDegrees(angle_x));
    skew.Set(1, 0, TanDegrees(angle_y));
    matrix_.PreConcat(skew);
  }
}

void Transform::ApplyPerspectiveDepth(float depth) {
  if (depth == 0) {
    return;
  }
  if (matrix_.IsIdentity()) {
    matrix_.Set(3, 2, -1.f / depth);
  } else {
    skity::Matrix m;
    m.Set(3, 2, -1.f / depth);
    matrix_.PreConcat(m);
  }
}

void Transform::PreconcatTransform(const Transform& transform) {
  matrix_ = matrix_ * transform.matrix_;
}

void Transform::ConcatTransform(const Transform& transform) {
  matrix_.PostConcat(transform.matrix_);
}

bool Transform::IsApproximatelyIdentityOrTranslation(float tolerance) const {
  FML_DCHECK(tolerance >= 0);
  return ApproximatelyOne(matrix_.Get(0, 0), tolerance) &&
         ApproximatelyZero(matrix_.Get(1, 0), tolerance) &&
         ApproximatelyZero(matrix_.Get(2, 0), tolerance) &&
         matrix_.Get(3, 0) == 0 &&
         ApproximatelyZero(matrix_.Get(0, 1), tolerance) &&
         ApproximatelyOne(matrix_.Get(1, 1), tolerance) &&
         ApproximatelyZero(matrix_.Get(2, 1), tolerance) &&
         matrix_.Get(3, 1) == 0 &&
         ApproximatelyZero(matrix_.Get(0, 2), tolerance) &&
         ApproximatelyZero(matrix_.Get(1, 2), tolerance) &&
         ApproximatelyOne(matrix_.Get(2, 2), tolerance) &&
         matrix_.Get(3, 2) == 0 && matrix_.Get(3, 3) == 1;
}

bool Transform::IsApproximatelyIdentityOrIntegerTranslation(
    float tolerance) const {
  if (!IsApproximatelyIdentityOrTranslation(tolerance)) {
    return false;
  }

  for (float t : {matrix_.Get(0, 3), matrix_.Get(1, 3), matrix_.Get(2, 3)}) {
    if (!IsValueInRangeForNumericType<int>(t) ||
        std::abs(std::round(t) - t) > tolerance) {
      return false;
    }
  }
  return true;
}

bool Transform::IsIdentityOrIntegerTranslation() const {
  if (!IsIdentityOrTranslation()) {
    return false;
  }

  for (float t : {matrix_.Get(0, 3), matrix_.Get(1, 3), matrix_.Get(2, 3)}) {
    if (!IsValueInRangeForNumericType<int>(t) || static_cast<int>(t) != t) {
      return false;
    }
  }
  return true;
}

bool Transform::IsBackFaceVisible() const {
  // Compute whether a layer with a forward-facing normal of (0, 0, 1, 0)
  // would have its back face visible after applying the transform.
  if (matrix_.IsIdentity()) {
    return false;
  }

  // This is done by transforming the normal and seeing if the resulting z
  // value is positive or negative. However, note that transforming a normal
  // actually requires using the inverse-transpose of the original transform.
  //
  // We can avoid inverting and transposing the matrix since we know we want
  // to transform only the specific normal vector (0, 0, 1, 0). In this case,
  // we only need the 3rd row, 3rd column of the inverse-transpose. We can
  // calculate only the 3rd row 3rd column element of the inverse, skipping
  // everything else.
  //
  // For more information, refer to:
  //   http://en.wikipedia.org/wiki/Invertible_matrix#Analytic_solution
  //

  double determinant = matrix_.Determinant();

  // If matrix was not invertible, then just assume back face is not visible.
  if (determinant == 0) {
    return false;
  }

  // Compute the cofactor of the 3rd row, 3rd column.
  double cofactor_part_1 =
      matrix_.Get(0, 0) * matrix_.Get(1, 1) * matrix_.Get(3, 3);

  double cofactor_part_2 =
      matrix_.Get(0, 1) * matrix_.Get(1, 3) * matrix_.Get(3, 0);

  double cofactor_part_3 =
      matrix_.Get(0, 3) * matrix_.Get(1, 0) * matrix_.Get(3, 1);

  double cofactor_part_4 =
      matrix_.Get(0, 0) * matrix_.Get(1, 3) * matrix_.Get(3, 1);

  double cofactor_part_5 =
      matrix_.Get(0, 1) * matrix_.Get(1, 0) * matrix_.Get(3, 3);

  double cofactor_part_6 =
      matrix_.Get(0, 3) * matrix_.Get(1, 1) * matrix_.Get(3, 0);

  double cofactor33 = cofactor_part_1 + cofactor_part_2 + cofactor_part_3 -
                      cofactor_part_4 - cofactor_part_5 - cofactor_part_6;

  // Technically the transformed z component is cofactor33 / determinant.  But
  // we can avoid the costly division because we only care about the resulting
  // +/- sign; we can check this equivalently by multiplication.
  return cofactor33 * determinant < -kEpsilon;
}

bool Transform::GetInverse(Transform* transform) const {
  if (!matrix_.Invert(&transform->matrix_)) {
    // Initialize the return value to identity if this matrix turned
    // out to be un-invertible.
    transform->MakeIdentity();
    return false;
  }

  return true;
}

bool Transform::Preserves2dAxisAlignment() const {
  // Check whether an axis aligned 2-dimensional rect would remain axis-aligned
  // after being transformed by this matrix (and implicitly projected by
  // dropping any non-zero z-values).
  //
  // The 4th column can be ignored because translations don't affect axis
  // alignment. The 3rd column can be ignored because we are assuming 2d
  // inputs, where z-values will be zero. The 3rd row can also be ignored
  // because we are assuming 2d outputs, and any resulting z-value is dropped
  // anyway. For the inner 2x2 portion, the only effects that keep a rect axis
  // aligned are (1) swapping axes and (2) scaling axes. This can be checked by
  // verifying only 1 element of every column and row is non-zero.  Degenerate
  // cases that project the x or y dimension to zero are considered to preserve
  // axis alignment.
  //
  // If the matrix does have perspective component that is affected by x or y
  // values: The current implementation conservatively assumes that axis
  // alignment is not preserved.

  bool has_x_or_y_perspective =
      matrix_.Get(3, 0) != 0 || matrix_.Get(3, 1) != 0;

  int num_non_zero_in_row_0 = 0;
  int num_non_zero_in_row_1 = 0;
  int num_non_zero_in_col_0 = 0;
  int num_non_zero_in_col_1 = 0;

  if (std::abs(matrix_.Get(0, 0)) > kEpsilon) {
    num_non_zero_in_row_0++;
    num_non_zero_in_col_0++;
  }

  if (std::abs(matrix_.Get(0, 1)) > kEpsilon) {
    num_non_zero_in_row_0++;
    num_non_zero_in_col_1++;
  }

  if (std::abs(matrix_.Get(1, 0)) > kEpsilon) {
    num_non_zero_in_row_1++;
    num_non_zero_in_col_0++;
  }

  if (std::abs(matrix_.Get(1, 1)) > kEpsilon) {
    num_non_zero_in_row_1++;
    num_non_zero_in_col_1++;
  }

  return num_non_zero_in_row_0 <= 1 && num_non_zero_in_row_1 <= 1 &&
         num_non_zero_in_col_0 <= 1 && num_non_zero_in_col_1 <= 1 &&
         !has_x_or_y_perspective;
}

void Transform::Transpose() { matrix_.Transpose(); }

void Transform::FlattenTo2d() {
  matrix_.Set(2, 0, 0.0);
  matrix_.Set(2, 1, 0.0);
  matrix_.Set(0, 2, 0.0);
  matrix_.Set(1, 2, 0.0);
  matrix_.Set(2, 2, 1.0);
  matrix_.Set(3, 2, 0.0);
  matrix_.Set(2, 3, 0.0);
}

bool Transform::IsFlat() const {
  return matrix_.Get(2, 0) == 0.0 && matrix_.Get(2, 1) == 0.0 &&
         matrix_.Get(0, 2) == 0.0 && matrix_.Get(1, 2) == 0.0 &&
         matrix_.Get(2, 2) == 1.0 && matrix_.Get(3, 2) == 0.0 &&
         matrix_.Get(2, 3) == 0.0;
}

FloatVector2d Transform::To2dTranslation() const {
  return FloatVector2d(matrix_.Get(0, 3), matrix_.Get(1, 3));
}

void Transform::TransformPoint(Point* point) const {
  FML_DCHECK(point);
  TransformPointInternal(matrix_, point);
}

void Transform::TransformPoint(FloatPoint* point) const {
  FML_DCHECK(point);
  TransformPointInternal(matrix_, point);
}

void Transform::TransformPoint(FloatPoint3d* point) const {
  FML_DCHECK(point);
  TransformPointInternal(matrix_, point);
}

void Transform::TransformVector(FloatVector3d* vector) const {
  FML_DCHECK(vector);
  TransformVectorInternal(matrix_, vector);
}

bool Transform::TransformPointReverse(Point* point) const {
  FML_DCHECK(point);

  // TODO(sad): Try to avoid trying to invert the matrix.
  skity::Matrix inverse;
  if (!matrix_.Invert(&inverse)) {
    return false;
  }

  TransformPointInternal(inverse, point);
  return true;
}

bool Transform::TransformPointReverse(FloatPoint* point) const {
  FML_DCHECK(point);

  // TODO(sad): Try to avoid trying to invert the matrix.
  skity::Matrix inverse;
  if (!matrix_.Invert(&inverse)) {
    return false;
  }

  TransformPointInternal(inverse, point);
  return true;
}

bool Transform::TransformPointReverse(FloatPoint3d* point) const {
  FML_DCHECK(point);

  // TODO(sad): Try to avoid trying to invert the matrix.
  skity::Matrix inverse;
  if (!matrix_.Invert(&inverse)) {
    return false;
  }

  TransformPointInternal(inverse, point);
  return true;
}

bool Transform::Blend(const Transform& from, double progress) {
  DecomposedTransform to_decomp;
  DecomposedTransform from_decomp;
  if (!DecomposeTransform(&to_decomp, *this) ||
      !DecomposeTransform(&from_decomp, from)) {
    return false;
  }

  to_decomp = BlendDecomposedTransforms(to_decomp, from_decomp, progress);

  matrix_ = ComposeTransform(to_decomp).matrix();
  return true;
}

void Transform::RoundTranslationComponents() {
  matrix_.Set(0, 3, std::round(matrix_.Get(0, 3)));
  matrix_.Set(1, 3, std::round(matrix_.Get(1, 3)));
}

void Transform::TransformPointInternal(const skity::Matrix& xform,
                                       FloatPoint3d* point) const {
  if (xform.IsIdentity()) {
    return;
  }

  skity::Vec4 p = {point->x(), point->y(), point->z(), 1};

  p = xform * p;
  if (p[3] != 1.f && p[3] != 0.f) {
    float w_inverse = 1.f / p[3];
    point->SetPoint(p[0] * w_inverse, p[1] * w_inverse, p[2] * w_inverse);
  } else {
    point->SetPoint(p[0], p[1], p[2]);
  }
}

void Transform::TransformVectorInternal(const skity::Matrix& xform,
                                        FloatVector3d* vector) const {
  if (xform.IsIdentity()) {
    return;
  }

  skity::Vec4 p = {vector->x(), vector->y(), vector->z(), 0};

  p = xform * p;

  vector->SetX(p[0]);
  vector->SetY(p[1]);
  vector->SetZ(p[2]);
}

void Transform::TransformPointInternal(const skity::Matrix& xform,
                                       FloatPoint* point) const {
  if (xform.IsIdentity()) {
    return;
  }

  skity::Vec4 p = {point->x(), point->y(), 0, 1};

  p = xform * p;

  point->SetX(p[0]);
  point->SetY(p[1]);
}

void Transform::TransformPointInternal(const skity::Matrix& xform,
                                       Point* point) const {
  FloatPoint point_float(*point);
  TransformPointInternal(xform, &point_float);
  point->SetX(std::lroundf(point_float.x()));
  point->SetY(std::lroundf(point_float.y()));
}

bool Transform::ApproximatelyEqual(const Transform& transform) const {
  static const float component_tolerance = 0.1f;

  // We may have a larger discrepancy in the scroll components due to snapping
  // (floating point error might round the other way).
  static const float translation_tolerance = 1.f;

  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      const float delta =
          std::abs(matrix().Get(row, col) - transform.matrix().Get(row, col));
      const float tolerance =
          col == 3 && row < 3 ? translation_tolerance : component_tolerance;
      if (delta > tolerance) {
        return false;
      }
    }
  }

  return true;
}

std::string Transform::ToString() const {
  auto str_printf = [=](char* str_buf, size_t size) {
    return std::snprintf(
        str_buf, size,
        "[ %+0.4f %+0.4f %+0.4f %+0.4f  \n"
        "  %+0.4f %+0.4f %+0.4f %+0.4f  \n"
        "  %+0.4f %+0.4f %+0.4f %+0.4f  \n"
        "  %+0.4f %+0.4f %+0.4f %+0.4f ]\n",
        matrix_.Get(0, 0), matrix_.Get(0, 1), matrix_.Get(0, 2),
        matrix_.Get(0, 3), matrix_.Get(1, 0), matrix_.Get(1, 1),
        matrix_.Get(1, 2), matrix_.Get(1, 3), matrix_.Get(2, 0),
        matrix_.Get(2, 1), matrix_.Get(2, 2), matrix_.Get(2, 3),
        matrix_.Get(3, 0), matrix_.Get(3, 1), matrix_.Get(3, 2),
        matrix_.Get(3, 3));
  };
  int sz = str_printf(nullptr, 0);
  std::vector<char> buf(sz + 1);
  str_printf(&buf[0], buf.size());
  return std::string(buf.data());
}

}  // namespace clay
