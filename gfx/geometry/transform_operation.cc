// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/geometry/transform_operation.h"

#include <cmath>

#include "base/include/log/logging.h"

namespace lynx {
namespace gfx {

namespace {

bool IsOperationIdentity(const TransformOperation* operation) {
  return !operation || operation->IsIdentity();
}

float GetDefaultValue(TransformOperation::Type type) {
  return type == TransformOperation::Type::kScale ? 1.0f : 0.0f;
}

bool IsIdentityMatrix(const std::array<float, 16>& matrix) {
  static constexpr std::array<float, 16> identity_matrix = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  return matrix == identity_matrix;
}

LengthUnit GetTranslateResultUnit(
    const TransformOperation* from, const TransformOperation* to,
    LengthValue TransformOperation::Translate::*axis) {
  if (IsOperationIdentity(from)) {
    return (to->translate.*axis).unit;
  }
  if (IsOperationIdentity(to)) {
    return (from->translate.*axis).unit;
  }
  return (from->translate.*axis).unit == LengthUnit::kPercent &&
                 (to->translate.*axis).unit == LengthUnit::kPercent
             ? LengthUnit::kPercent
             : LengthUnit::kNumber;
}

float BlendValue(float from, float to, float progress) {
  return from * (1.0f - progress) + to * progress;
}

bool FloatsApproximatelyEqual(float left, float right, float tolerance) {
  return std::fabs(left - right) <= tolerance;
}

float TranslateValue(const TransformOperation* operation,
                     LengthValue TransformOperation::Translate::*axis,
                     float percentage_base, LengthUnit result_unit) {
  if (IsOperationIdentity(operation)) {
    return 0.0f;
  }
  const LengthValue& value = operation->translate.*axis;
  return result_unit == LengthUnit::kPercent ? value.value
                                             : value.Resolve(percentage_base);
}

TransformOperation ComposeTransform(const DecomposedTransform& decomposed) {
  TransformOperation result;
  result.type = TransformOperation::kMatrix3d;
  Matrix44 matrix;

  for (int i = 0; i < 3; ++i) {
    if (decomposed.perspective[i] != 0) {
      matrix.setRC(3, i, decomposed.perspective[i]);
    }
  }
  if (decomposed.perspective[3] != 1) {
    matrix.setRC(3, 3, decomposed.perspective[3]);
  }

  matrix.preTranslate(decomposed.translate[0], decomposed.translate[1],
                      decomposed.translate[2]);
  matrix.preConcat(Matrix44(decomposed.quaternion));

  if (decomposed.skew[2] != 0.0f) {
    Matrix44 skew_yz;
    skew_yz.setRC(1, 2, decomposed.skew[2]);
    matrix.preConcat(skew_yz);
  }
  if (decomposed.skew[1] != 0.0f) {
    Matrix44 skew_xz;
    skew_xz.setRC(0, 2, decomposed.skew[1]);
    matrix.preConcat(skew_xz);
  }
  if (decomposed.skew[0] != 0.0f) {
    Matrix44 skew_xy;
    skew_xy.setRC(0, 1, decomposed.skew[0]);
    matrix.preConcat(skew_xy);
  }

  matrix.preScale(decomposed.scale[0], decomposed.scale[1],
                  decomposed.scale[2]);

  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      // CSS matrix3d values and Matrix44::Matrix() are column-major.
      result.matrix.matrix_data.at(4 * col + row) = matrix.rc(row, col);
    }
  }
  return result;
}

}  // namespace

bool TransformOperation::IsIdentity() const {
  switch (type) {
    case Type::kTranslate:
      return translate.x.value == 0.0f && translate.y.value == 0.0f &&
             translate.z.value == 0.0f;
    case Type::kRotateX:
    case Type::kRotateY:
    case Type::kRotateZ:
      return rotate.degree == GetDefaultValue(type);
    case Type::kScale:
      return scale.x == GetDefaultValue(type) &&
             scale.y == GetDefaultValue(type);
    case Type::kSkew:
      return skew.x == GetDefaultValue(type) && skew.y == GetDefaultValue(type);
    case Type::kMatrix:
    case Type::kMatrix3d:
      return IsIdentityMatrix(matrix.matrix_data);
    default:
      return true;
  }
}

bool TransformOperation::ApproximatelyEqual(const TransformOperation& other,
                                            float tolerance) const {
  DCHECK(tolerance >= 0.0f);
  if (type != other.type) {
    return false;
  }
  switch (type) {
    case Type::kTranslate:
      return translate.x.unit == other.translate.x.unit &&
             translate.y.unit == other.translate.y.unit &&
             translate.z.unit == other.translate.z.unit &&
             FloatsApproximatelyEqual(translate.x.value,
                                      other.translate.x.value, tolerance) &&
             FloatsApproximatelyEqual(translate.y.value,
                                      other.translate.y.value, tolerance) &&
             FloatsApproximatelyEqual(translate.z.value,
                                      other.translate.z.value, tolerance);
    case Type::kRotateX:
    case Type::kRotateY:
    case Type::kRotateZ:
      return FloatsApproximatelyEqual(rotate.degree, other.rotate.degree,
                                      tolerance);
    case Type::kScale:
      return FloatsApproximatelyEqual(scale.x, other.scale.x, tolerance) &&
             FloatsApproximatelyEqual(scale.y, other.scale.y, tolerance);
    case Type::kSkew:
      return FloatsApproximatelyEqual(skew.x, other.skew.x, tolerance) &&
             FloatsApproximatelyEqual(skew.y, other.skew.y, tolerance);
    case Type::kMatrix:
    case Type::kMatrix3d: {
      for (size_t i = 0; i < matrix.matrix_data.size(); ++i) {
        if (!FloatsApproximatelyEqual(matrix.matrix_data[i],
                                      other.matrix.matrix_data[i], tolerance)) {
          return false;
        }
      }
      return true;
    }
    default:
      return true;
  }
}

Matrix44 TransformOperation::GetMatrix(float reference_width,
                                       float reference_height) const {
  Matrix44 result;
  switch (type) {
    case Type::kTranslate:
      result.preTranslate(translate.x.Resolve(reference_width),
                          translate.y.Resolve(reference_height),
                          translate.z.Resolve(0.0f));
      break;
    case Type::kRotateX:
      result.setRotateAboutXAxis(rotate.degree);
      break;
    case Type::kRotateY:
      result.setRotateAboutYAxis(rotate.degree);
      break;
    case Type::kRotateZ:
      result.setRotateAboutZAxis(rotate.degree);
      break;
    case Type::kScale:
      result.preScale(scale.x, scale.y, 1.0f);
      break;
    case Type::kSkew:
      result.Skew(skew.x, skew.y);
      break;
    case Type::kMatrix:
    case Type::kMatrix3d:
      result.Matrix(matrix.matrix_data);
      break;
    default:
      break;
  }
  return result;
}

bool TransformOperation::BlendTransformOperations(
    const TransformOperation* from, const TransformOperation* to,
    float progress, float reference_width, float reference_height,
    TransformOperation* result) {
  if (!result) {
    return false;
  }
  if (!from && !to) {
    *result = TransformOperation();
    return true;
  }
  DCHECK(from || to);
  if (IsOperationIdentity(from) && IsOperationIdentity(to)) {
    *result = TransformOperation();
    return true;
  }

  TransformOperation operation;
  operation.type = IsOperationIdentity(from) ? to->type : from->type;
  switch (operation.type) {
    case Type::kTranslate: {
      const LengthUnit x_unit =
          GetTranslateResultUnit(from, to, &TransformOperation::Translate::x);
      const LengthUnit y_unit =
          GetTranslateResultUnit(from, to, &TransformOperation::Translate::y);
      const float from_x = TranslateValue(
          from, &TransformOperation::Translate::x, reference_width, x_unit);
      const float to_x = TranslateValue(to, &TransformOperation::Translate::x,
                                        reference_width, x_unit);
      const float from_y = TranslateValue(
          from, &TransformOperation::Translate::y, reference_height, y_unit);
      const float to_y = TranslateValue(to, &TransformOperation::Translate::y,
                                        reference_height, y_unit);
      // Preserve legacy transform interpolation semantics: blend translateZ
      // percentages as raw scalars, then emit an absolute value.
      const float from_z =
          IsOperationIdentity(from) ? 0.0f : from->translate.z.value;
      const float to_z = IsOperationIdentity(to) ? 0.0f : to->translate.z.value;

      operation.translate.x = {BlendValue(from_x, to_x, progress), x_unit};
      operation.translate.y = {BlendValue(from_y, to_y, progress), y_unit};
      operation.translate.z = {BlendValue(from_z, to_z, progress),
                               LengthUnit::kNumber};
      *result = operation;
      return true;
    }
    case Type::kRotateX:
    case Type::kRotateY:
    case Type::kRotateZ: {
      const float from_value =
          IsOperationIdentity(from) ? 0.0f : from->rotate.degree;
      const float to_value = IsOperationIdentity(to) ? 0.0f : to->rotate.degree;
      operation.rotate.degree = BlendValue(from_value, to_value, progress);
      *result = operation;
      return true;
    }
    case Type::kScale: {
      const float from_x = IsOperationIdentity(from) ? 1.0f : from->scale.x;
      const float from_y = IsOperationIdentity(from) ? 1.0f : from->scale.y;
      const float to_x = IsOperationIdentity(to) ? 1.0f : to->scale.x;
      const float to_y = IsOperationIdentity(to) ? 1.0f : to->scale.y;
      operation.scale.x = BlendValue(from_x, to_x, progress);
      operation.scale.y = BlendValue(from_y, to_y, progress);
      *result = operation;
      return true;
    }
    case Type::kSkew: {
      const float from_x = IsOperationIdentity(from) ? 0.0f : from->skew.x;
      const float from_y = IsOperationIdentity(from) ? 0.0f : from->skew.y;
      const float to_x = IsOperationIdentity(to) ? 0.0f : to->skew.x;
      const float to_y = IsOperationIdentity(to) ? 0.0f : to->skew.y;
      operation.skew.x = BlendValue(from_x, to_x, progress);
      operation.skew.y = BlendValue(from_y, to_y, progress);
      *result = operation;
      return true;
    }
    case Type::kMatrix:
    case Type::kMatrix3d: {
      DecomposedTransform from_decomposed;
      Matrix44 from_matrix;
      if (!IsOperationIdentity(from)) {
        from_matrix.Matrix(from->matrix.matrix_data);
      }
      if (!DecomposeTransform(&from_decomposed, from_matrix)) {
        return false;
      }

      DecomposedTransform to_decomposed;
      Matrix44 to_matrix;
      if (!IsOperationIdentity(to)) {
        to_matrix.Matrix(to->matrix.matrix_data);
      }
      if (!DecomposeTransform(&to_decomposed, to_matrix)) {
        return false;
      }
      *result = ComposeTransform(
          BlendDecomposedTransforms(to_decomposed, from_decomposed, progress));
      return true;
    }
    default:
      *result = operation;
      return true;
  }
}

TransformOperation TransformOperation::FromDecomposedTransform(
    const DecomposedTransform& decomposed_transform) {
  return ComposeTransform(decomposed_transform);
}

}  // namespace gfx
}  // namespace lynx
