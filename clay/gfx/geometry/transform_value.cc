// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/transform_value.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "clay/public/style_types.h"

namespace clay {

namespace {

constexpr lynx::gfx::LengthValue kZeroLength{0.0f,
                                             lynx::gfx::LengthUnit::kNumber};

lynx::gfx::LengthValue Number(float value) {
  return {value, lynx::gfx::LengthUnit::kNumber};
}

std::array<double, 16> ToMatrixData(const double matrix[16]) {
  std::array<double, 16> result;
  std::copy_n(matrix, result.size(), result.begin());
  return result;
}

void AppendRawOperation(TransformValue& result, const TransformRaw& raw,
                        float width, float height) {
  const auto value = [&](size_t index, float percentage_base) {
    return raw.values[index].GetValue(percentage_base);
  };

  switch (static_cast<ClayTransformType>(raw.type)) {
    case ClayTransformType::kTranslate:
      result.visual_operations.AppendTranslate(
          Number(value(0, width)), Number(value(1, height)), kZeroLength);
      break;
    case ClayTransformType::kTranslateX:
      result.visual_operations.AppendTranslate(Number(value(0, width)),
                                               kZeroLength, kZeroLength);
      break;
    case ClayTransformType::kTranslateY:
      result.visual_operations.AppendTranslate(
          kZeroLength, Number(value(0, height)), kZeroLength);
      break;
    case ClayTransformType::kTranslateZ:
      result.visual_operations.AppendTranslate(kZeroLength, kZeroLength,
                                               kZeroLength);
      result.stacking_z += value(0, 0.0f);
      break;
    case ClayTransformType::kTranslate3d:
      result.visual_operations.AppendTranslate(
          Number(value(0, width)), Number(value(1, height)), kZeroLength);
      result.stacking_z += value(2, 0.0f);
      break;
    case ClayTransformType::kRotateX:
      result.visual_operations.AppendRotate(
          lynx::gfx::TransformOperation::kRotateX, value(0, 0.0f));
      break;
    case ClayTransformType::kRotateY:
      result.visual_operations.AppendRotate(
          lynx::gfx::TransformOperation::kRotateY, value(0, 0.0f));
      break;
    case ClayTransformType::kRotate:
    case ClayTransformType::kRotateZ:
      result.visual_operations.AppendRotate(
          lynx::gfx::TransformOperation::kRotateZ, value(0, 0.0f));
      break;
    case ClayTransformType::kScale:
      result.visual_operations.AppendScale(value(0, 0.0f), value(1, 0.0f));
      break;
    case ClayTransformType::kScaleX:
      result.visual_operations.AppendScale(value(0, 0.0f), 1.0f);
      break;
    case ClayTransformType::kScaleY:
      result.visual_operations.AppendScale(1.0f, value(0, 0.0f));
      break;
    case ClayTransformType::kSkew:
      result.visual_operations.AppendSkew(value(0, 0.0f), value(1, 0.0f));
      break;
    case ClayTransformType::kSkewX:
      result.visual_operations.AppendSkew(value(0, 0.0f), 0.0f);
      break;
    case ClayTransformType::kSkewY:
      result.visual_operations.AppendSkew(0.0f, value(0, 0.0f));
      break;
    case ClayTransformType::kMatrix:
      result.visual_operations.AppendMatrix(
          lynx::gfx::TransformOperation::kMatrix, ToMatrixData(raw.matrix));
      break;
    case ClayTransformType::kMatrix3d:
      result.visual_operations.AppendMatrix(
          lynx::gfx::TransformOperation::kMatrix3d, ToMatrixData(raw.matrix));
      break;
    case ClayTransformType::kNone:
      break;
  }
}

}  // namespace

bool TransformValue::ApproximatelyEqual(const TransformValue& other,
                                        float tolerance) const {
  return std::abs(stacking_z - other.stacking_z) <= tolerance &&
         visual_operations.ApproximatelyEqual(other.visual_operations,
                                              tolerance);
}

TransformValue TransformValue::Blend(const TransformValue& from, float progress,
                                     float reference_width,
                                     float reference_height) const {
  TransformValue result;
  if (visual_operations.TryBlend(from.visual_operations, progress,
                                 reference_width, reference_height,
                                 &result.visual_operations)) {
    result.stacking_z =
        from.stacking_z * (1.0f - progress) + stacking_z * progress;
  } else {
    result = progress < 0.5f ? from : *this;
  }
  return result;
}

TransformValue ResolveTransform(const std::vector<TransformRaw>& transform_raw,
                                float width, float height) {
  TransformValue result;
  for (const auto& raw : transform_raw) {
    AppendRawOperation(result, raw, width, height);
  }
  return result;
}

TransformValue ResolveTransform(
    const lynx::gfx::TransformOperations& operations, float width,
    float height) {
  TransformValue result;
  for (const auto& operation : operations.GetOperations()) {
    if (operation.type != lynx::gfx::TransformOperation::kTranslate) {
      result.visual_operations.Append(operation);
      continue;
    }
    result.visual_operations.AppendTranslate(
        Number(operation.translate.x.Resolve(width)),
        Number(operation.translate.y.Resolve(height)), kZeroLength);
    result.stacking_z += operation.translate.z.Resolve(0.0f);
  }
  return result;
}

skity::Matrix ToSkityMatrix(const lynx::gfx::Matrix44& matrix) {
  skity::Matrix result;
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      result.Set(row, column, matrix.rc(row, column));
    }
  }
  return result;
}

skity::Matrix ApplyTransform(
    const lynx::gfx::TransformOperations& visual_operations, float perspective,
    float origin_x, float origin_y, float offset_x, float offset_y) {
  lynx::gfx::Matrix44 matrix;
  if (std::abs(perspective) > 1e-6f) {
    matrix.setRC(3, 2, -1.0f / perspective);
  }
  matrix.preConcat(visual_operations.ApplyRemaining(0, 0.0f, 0.0f));
  return ToSkityMatrix(matrix)
      .PreTranslate(-origin_x, -origin_y)
      .PostTranslate(origin_x, origin_y)
      .PostTranslate(offset_x, offset_y);
}

}  // namespace clay
