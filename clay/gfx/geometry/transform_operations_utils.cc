// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/transform_operations_utils.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "clay/public/style_types.h"
#include "gfx/geometry/matrix44.h"

namespace clay {

namespace {

constexpr lynx::gfx::LengthValue kZeroLength{};

lynx::gfx::LengthValue Number(float value) {
  return {value, lynx::gfx::LengthUnit::kNumber};
}

std::array<double, 16> ToMatrixData(const double matrix[16]) {
  std::array<double, 16> result;
  std::copy_n(matrix, result.size(), result.begin());
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

void AppendRawOperation(lynx::gfx::TransformOperations& result,
                        const TransformRaw& raw, float width, float height) {
  const auto value = [&](size_t index, float percentage_base) {
    return raw.values[index].GetValue(percentage_base);
  };

  switch (static_cast<ClayTransformType>(raw.type)) {
    case ClayTransformType::kTranslate:
      result.AppendTranslate(Number(value(0, width)), Number(value(1, height)),
                             kZeroLength);
      break;
    case ClayTransformType::kTranslateX:
      result.AppendTranslate(Number(value(0, width)), kZeroLength, kZeroLength);
      break;
    case ClayTransformType::kTranslateY:
      result.AppendTranslate(kZeroLength, Number(value(0, height)),
                             kZeroLength);
      break;
    case ClayTransformType::kTranslateZ:
      result.AppendTranslate(kZeroLength, kZeroLength, Number(value(0, 0.0f)));
      break;
    case ClayTransformType::kTranslate3d:
      result.AppendTranslate(Number(value(0, width)), Number(value(1, height)),
                             Number(value(2, 0.0f)));
      break;
    case ClayTransformType::kRotateX:
      result.AppendRotate(lynx::gfx::TransformOperation::kRotateX,
                          value(0, 0.0f));
      break;
    case ClayTransformType::kRotateY:
      result.AppendRotate(lynx::gfx::TransformOperation::kRotateY,
                          value(0, 0.0f));
      break;
    case ClayTransformType::kRotate:
    case ClayTransformType::kRotateZ:
      result.AppendRotate(lynx::gfx::TransformOperation::kRotateZ,
                          value(0, 0.0f));
      break;
    case ClayTransformType::kScale:
      result.AppendScale(value(0, 0.0f), value(1, 0.0f));
      break;
    case ClayTransformType::kScaleX:
      result.AppendScale(value(0, 0.0f), 1.0f);
      break;
    case ClayTransformType::kScaleY:
      result.AppendScale(1.0f, value(0, 0.0f));
      break;
    case ClayTransformType::kSkew:
      result.AppendSkew(value(0, 0.0f), value(1, 0.0f));
      break;
    case ClayTransformType::kSkewX:
      result.AppendSkew(value(0, 0.0f), 0.0f);
      break;
    case ClayTransformType::kSkewY:
      result.AppendSkew(0.0f, value(0, 0.0f));
      break;
    case ClayTransformType::kMatrix:
      result.AppendMatrix(lynx::gfx::TransformOperation::kMatrix,
                          ToMatrixData(raw.matrix));
      break;
    case ClayTransformType::kMatrix3d:
      result.AppendMatrix(lynx::gfx::TransformOperation::kMatrix3d,
                          ToMatrixData(raw.matrix));
      break;
    case ClayTransformType::kNone:
      break;
  }
}

}  // namespace

lynx::gfx::TransformOperations ResolveTransform(
    const std::vector<TransformRaw>& transform_raw, float width, float height) {
  lynx::gfx::TransformOperations result;
  for (const auto& raw : transform_raw) {
    AppendRawOperation(result, raw, width, height);
  }
  return result;
}

lynx::gfx::TransformOperations ResolveTransform(
    const lynx::gfx::TransformOperations& operations, float width,
    float height) {
  lynx::gfx::TransformOperations result;
  for (const auto& operation : operations.GetOperations()) {
    if (operation.type != lynx::gfx::TransformOperation::kTranslate) {
      result.Append(operation);
      continue;
    }
    result.AppendTranslate(Number(operation.translate.x.Resolve(width)),
                           Number(operation.translate.y.Resolve(height)),
                           Number(operation.translate.z.Resolve(0.0f)));
  }
  return result;
}

float GetTranslateZ(const lynx::gfx::TransformOperations& operations) {
  float result = 0.0f;
  for (const auto& operation : operations.GetOperations()) {
    if (operation.type == lynx::gfx::TransformOperation::kTranslate) {
      result += operation.translate.z.Resolve(0.0f);
    }
  }
  return result;
}

skity::Matrix ApplyTransform(const lynx::gfx::TransformOperations& operations,
                             float perspective, float origin_x, float origin_y,
                             float offset_x, float offset_y) {
  lynx::gfx::Matrix44 matrix;
  if (std::abs(perspective) > 1e-6f) {
    matrix.setRC(3, 2, -1.0f / perspective);
  }
  for (const auto& operation : operations.GetOperations()) {
    if (operation.type == lynx::gfx::TransformOperation::kTranslate) {
      lynx::gfx::Matrix44 translation;
      translation.preTranslate(operation.translate.x.Resolve(0.0f),
                               operation.translate.y.Resolve(0.0f), 0.0f);
      matrix.preConcat(translation);
    } else {
      matrix.preConcat(operation.GetMatrix(0.0f, 0.0f));
    }
  }
  return ToSkityMatrix(matrix)
      .PreTranslate(-origin_x, -origin_y)
      .PostTranslate(origin_x, origin_y)
      .PostTranslate(offset_x, offset_y);
}

}  // namespace clay
