// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/transform_operations.h"

#include <stddef.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/geometry/float_vector3d.h"
#include "clay/gfx/geometry/math_util.h"
#include "clay/gfx/geometry/transform_util.h"

namespace clay {

TransformOperations::TransformOperations() = default;

TransformOperations::TransformOperations(const TransformOperations& other) {
  operations_ = other.operations_;
  perspective_ = other.perspective_;
}

TransformOperations::~TransformOperations() = default;

TransformOperations& TransformOperations::operator=(
    const TransformOperations& other) {
  operations_ = other.operations_;
  perspective_ = other.perspective_;
  return *this;
}

TransformOperations::TransformOperations(const ClayTransform& rk_transform,
                                         FloatSize percentage_resolution_size) {
#define GET_VALUE(index, relative_to)                    \
  (op.unit[index] == ClayPlatformLengthUnit::kPercentage \
       ? op.value[index] * relative_to                   \
       : op.value[index])
  auto width = percentage_resolution_size.width();
  auto height = percentage_resolution_size.height();

  for (int i = 0; i < rk_transform.size; ++i) {
    const ClayTransformOP& op = rk_transform.op[i];
    switch (op.type) {
      case ClayTransformType::kTranslate: {
        AppendTranslate(GET_VALUE(0, width), GET_VALUE(1, height), 0.0f);
      } break;
      case ClayTransformType::kTranslateX: {
        AppendTranslate(GET_VALUE(0, width), 0.0f, 0.0f);
      } break;
      case ClayTransformType::kTranslateY: {
        AppendTranslate(0.0f, GET_VALUE(0, height), 0.0f);
      } break;
      case ClayTransformType::kTranslateZ: {
        AppendTranslate(0.0f, 0.0f, GET_VALUE(0, 0));
      } break;
      case ClayTransformType::kTranslate3d: {
        AppendTranslate(GET_VALUE(0, width), GET_VALUE(1, height),
                        GET_VALUE(2, 0));
      } break;
      case ClayTransformType::kRotateX: {
        AppendRotate(1.0f, 0.0f, 0.0f, op.value[0]);
      } break;
      case ClayTransformType::kRotateY: {
        AppendRotate(0.0f, 1.0f, 0.0f, op.value[0]);
      } break;
      case ClayTransformType::kRotate:
      case ClayTransformType::kRotateZ: {
        AppendRotate(0.0f, 0.0f, 1.0f, op.value[0]);
      } break;
      case ClayTransformType::kScale: {
        AppendScale(op.value[0], op.value[1], 1.0f);
      } break;
      case ClayTransformType::kScaleX: {
        AppendScale(op.value[0], 1.0f, 1.0f);
      } break;
      case ClayTransformType::kScaleY: {
        AppendScale(1.0f, op.value[0], 1.0f);
      } break;
      case ClayTransformType::kSkew: {
        AppendSkew(op.value[0], op.value[1]);
      } break;
      case ClayTransformType::kSkewX: {
        AppendSkewX(op.value[0]);
      } break;
      case ClayTransformType::kSkewY: {
        AppendSkewY(op.value[0]);
      } break;
      case ClayTransformType::kMatrix:
      case ClayTransformType::kMatrix3d: {
        AppendMatrix(op.matrix);
      } break;
      case ClayTransformType::kNone:
      default:
        break;
    }
  }
#undef GET_VALUE
}

TransformOperations::TransformOperations(
    const std::vector<TransformRaw>& transform_raws, float width,
    float height) {
  for (const auto& transform_raw : transform_raws) {
    switch (static_cast<ClayTransformType>(transform_raw.type)) {
      case ClayTransformType::kTranslate: {
        AppendTranslate(transform_raw.values[0].GetValue(width),
                        transform_raw.values[1].GetValue(height), 0.0f);
      } break;
      case ClayTransformType::kTranslateX: {
        AppendTranslate(transform_raw.values[0].GetValue(width), 0.0f, 0.0f);
      } break;
      case ClayTransformType::kTranslateY: {
        AppendTranslate(0.0f, transform_raw.values[0].GetValue(height), 0.0f);
      } break;
      case ClayTransformType::kTranslateZ: {
        AppendTranslate(0.0f, 0.0f, transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kTranslate3d: {
        AppendTranslate(transform_raw.values[0].GetValue(width),
                        transform_raw.values[1].GetValue(height),
                        transform_raw.values[2].GetValue(0));
      } break;
      case ClayTransformType::kRotateX: {
        AppendRotate(1.0f, 0.0f, 0.0f, transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kRotateY: {
        AppendRotate(0.0f, 1.0f, 0.0f, transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kRotate:
      case ClayTransformType::kRotateZ: {
        AppendRotate(0.0f, 0.0f, 1.0f, transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kScale: {
        AppendScale(transform_raw.values[0].GetValue(0),
                    transform_raw.values[1].GetValue(0), 1.0f);
      } break;
      case ClayTransformType::kScaleX: {
        AppendScale(transform_raw.values[0].GetValue(0), 1.0f, 1.0f);
      } break;
      case ClayTransformType::kScaleY: {
        AppendScale(1.0f, transform_raw.values[0].GetValue(0), 1.0f);
      } break;
      case ClayTransformType::kSkew: {
        AppendSkew(transform_raw.values[0].GetValue(0),
                   transform_raw.values[1].GetValue(0));
      } break;
      case ClayTransformType::kSkewX: {
        AppendSkewX(transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kSkewY: {
        AppendSkewY(transform_raw.values[0].GetValue(0));
      } break;
      case ClayTransformType::kMatrix:
      case ClayTransformType::kMatrix3d: {
        AppendMatrix(transform_raw.matrix);
      } break;
      case ClayTransformType::kNone:
      default:
        break;
    }
  }
}

Transform TransformOperations::Apply() const {
  ApplyPerspective();
  return ApplyRemaining(0);
}

void TransformOperations::ApplyPerspective() const {
  if (fabsf(perspective_) <= 1e-6f) {
    return;
  }
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypePerspective;
  to_add.perspective_depth = perspective_;
  to_add.Bake();
  // Perspective should be at front.
  operations_.push_front(to_add);
  decomposed_transforms_.clear();
}

Transform TransformOperations::ApplyRemaining(size_t start) const {
  Transform to_return;
  for (size_t i = start; i < operations_.size(); i++) {
    to_return.PreconcatTransform(operations_[i].matrix);
  }
  return to_return;
}

TransformOperations TransformOperations::Blend(const TransformOperations& from,
                                               float progress) const {
  TransformOperations to_return;
  if (!BlendInternal(from, progress, &to_return)) {
    // If the matrices cannot be blended, fallback to discrete animation logic.
    // See https://drafts.csswg.org/css-transforms/#matrix-interpolation
    to_return = progress < 0.5 ? from : *this;
  }
  return to_return;
}

bool TransformOperations::PreservesAxisAlignment() const {
  for (const auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::kTypeIdentity:
      case TransformOperation::kTypeTranslate:
      case TransformOperation::kTypeScale:
        continue;
      case TransformOperation::kTypeMatrix:
      case TransformOperation::kTypeMatrix3d:
        if (!operation.matrix.IsIdentity() &&
            !operation.matrix.IsScaleOrTranslation()) {
          return false;
        }
        continue;
      case TransformOperation::kTypeRotate:
      case TransformOperation::kTypeSkewX:
      case TransformOperation::kTypeSkewY:
      case TransformOperation::kTypeSkew:
      case TransformOperation::kTypePerspective:
        return false;
    }
  }
  return true;
}

bool TransformOperations::IsTranslation() const {
  for (const auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::kTypeIdentity:
      case TransformOperation::kTypeTranslate:
        continue;
      case TransformOperation::kTypeMatrix:
      case TransformOperation::kTypeMatrix3d:
        if (!operation.matrix.IsIdentityOrTranslation()) {
          return false;
        }
        continue;
      case TransformOperation::kTypeRotate:
      case TransformOperation::kTypeScale:
      case TransformOperation::kTypeSkewX:
      case TransformOperation::kTypeSkewY:
      case TransformOperation::kTypeSkew:
      case TransformOperation::kTypePerspective:
        return false;
    }
  }
  return true;
}

static float TanDegrees(double degrees) { return std::tan(DegToRad(degrees)); }

bool TransformOperations::ScaleComponent(float* scale) const {
  float operations_scale = 1.f;
  for (const auto& operation : operations_) {
    switch (operation.type) {
      case TransformOperation::kTypeIdentity:
      case TransformOperation::kTypeTranslate:
      case TransformOperation::kTypeRotate:
        continue;
      case TransformOperation::kTypeMatrix:
      case TransformOperation::kTypeMatrix3d: {
        if (operation.matrix.HasPerspective()) {
          return false;
        }
        FloatVector2d scale_components =
            ComputeTransform2dScaleComponents(operation.matrix, 1.f);
        operations_scale *=
            std::max(scale_components.x(), scale_components.y());
        break;
      }
      case TransformOperation::kTypeSkewX:
      case TransformOperation::kTypeSkewY:
      case TransformOperation::kTypeSkew: {
        float x_component = TanDegrees(operation.skew.x);
        float y_component = TanDegrees(operation.skew.y);
        float x_scale = std::sqrt(x_component * x_component + 1);
        float y_scale = std::sqrt(y_component * y_component + 1);
        operations_scale *= std::max(x_scale, y_scale);
        break;
      }
      case TransformOperation::kTypePerspective:
        return false;
      case TransformOperation::kTypeScale:
        operations_scale *= std::max(
            std::abs(operation.scale.x),
            std::max(std::abs(operation.scale.y), std::abs(operation.scale.z)));
    }
  }
  *scale = operations_scale;
  return true;
}

bool TransformOperations::MatchesTypes(const TransformOperations& other) const {
  if (operations_.size() == 0 || other.operations_.size() == 0) {
    return true;
  }

  if (operations_.size() != other.operations_.size()) {
    return false;
  }

  for (size_t i = 0; i < operations_.size(); ++i) {
    if (operations_[i].type != other.operations_[i].type) {
      return false;
    }
  }

  return true;
}

size_t TransformOperations::MatchingPrefixLength(
    const TransformOperations& other) const {
  size_t num_operations =
      std::min(operations_.size(), other.operations_.size());
  for (size_t i = 0; i < num_operations; ++i) {
    if (operations_[i].type != other.operations_[i].type) {
      // Remaining operations in each operations list require matrix/matrix3d
      // interpolation.
      return i;
    }
  }
  // If the operations match to the length of the shorter list, then pad its
  // length with the matching identity operations.
  // https://drafts.csswg.org/css-transforms/#transform-function-lists
  return std::max(operations_.size(), other.operations_.size());
}

bool TransformOperations::CanBlendWith(const TransformOperations& other) const {
  TransformOperations dummy;
  return BlendInternal(other, 0.5, &dummy);
}

void TransformOperations::AppendTranslate(float x, float y, float z) {
  TransformOperation to_add;
  to_add.matrix.Translate3d(x, y, z);
  to_add.type = TransformOperation::kTypeTranslate;
  to_add.translate.x = x;
  to_add.translate.y = y;
  to_add.translate.z = z;
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendRotate(float x, float y, float z,
                                       float degrees) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypeRotate;
  to_add.rotate.axis.x = x;
  to_add.rotate.axis.y = y;
  to_add.rotate.axis.z = z;
  to_add.rotate.angle = degrees;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendScale(float x, float y, float z) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypeScale;
  to_add.scale.x = x;
  to_add.scale.y = y;
  to_add.scale.z = z;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendSkewX(float x) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypeSkewX;
  to_add.skew.x = x;
  to_add.skew.y = 0;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendSkewY(float y) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypeSkewY;
  to_add.skew.x = 0;
  to_add.skew.y = y;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendSkew(float x, float y) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypeSkew;
  to_add.skew.x = x;
  to_add.skew.y = y;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendPerspective(float depth) {
  TransformOperation to_add;
  to_add.type = TransformOperation::kTypePerspective;
  to_add.perspective_depth = depth;
  to_add.Bake();
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendMatrix(const double matrix[16]) {
  skity::Matrix m44(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4],
                    matrix[5], matrix[6], matrix[7], matrix[8], matrix[9],
                    matrix[10], matrix[11], matrix[12], matrix[13], matrix[14],
                    matrix[15]);
  Transform transform(m44);
  AppendMatrix(transform);
}

void TransformOperations::AppendMatrix(const Transform& matrix) {
  TransformOperation to_add;
  to_add.matrix = matrix;
  to_add.type = TransformOperation::kTypeMatrix;
  operations_.push_back(to_add);
  decomposed_transforms_.clear();
}

void TransformOperations::AppendIdentity() { operations_.emplace_back(); }

void TransformOperations::Append(const TransformOperation& operation) {
  operations_.push_back(operation);
  decomposed_transforms_.clear();
}

void TransformOperations::Append(const TransformOperations& ops) {
  for (const auto& operation : ops.operations_) {
    operations_.push_back(operation);
  }
  decomposed_transforms_.clear();
}

bool TransformOperations::IsIdentity() const {
  for (const auto& operation : operations_) {
    if (!operation.IsIdentity()) {
      return false;
    }
  }
  return true;
}

bool TransformOperations::ApproximatelyEqual(const TransformOperations& other,
                                             float tolerance) const {
  if (size() != other.size()) {
    return false;
  }
  for (size_t i = 0; i < operations_.size(); ++i) {
    if (!operations_[i].ApproximatelyEqual(other.operations_[i], tolerance)) {
      return false;
    }
  }
  return true;
}

bool TransformOperations::BlendInternal(const TransformOperations& from,
                                        float progress,
                                        TransformOperations* result) const {
  bool from_identity = from.IsIdentity();
  bool to_identity = IsIdentity();
  if (from_identity && to_identity) {
    return true;
  }

  size_t matching_prefix_length = MatchingPrefixLength(from);
  size_t from_size = from_identity ? 0 : from.operations_.size();
  size_t to_size = to_identity ? 0 : operations_.size();
  size_t num_operations = std::max(from_size, to_size);

  for (size_t i = 0; i < matching_prefix_length; ++i) {
    TransformOperation blended;
    if (!TransformOperation::BlendTransformOperations(
            i >= from_size ? nullptr : &from.operations_[i],
            i >= to_size ? nullptr : &operations_[i], progress, &blended)) {
      return false;
    }
    result->Append(blended);
  }

  if (matching_prefix_length < num_operations) {
    if (!ComputeDecomposedTransform(matching_prefix_length) ||
        !from.ComputeDecomposedTransform(matching_prefix_length)) {
      return false;
    }
    DecomposedTransform matrix_transform = BlendDecomposedTransforms(
        *decomposed_transforms_[matching_prefix_length].get(),
        *from.decomposed_transforms_[matching_prefix_length].get(), progress);
    result->AppendMatrix(ComposeTransform(matrix_transform));
  }
  return true;
}

bool TransformOperations::ComputeDecomposedTransform(
    size_t start_offset) const {
  auto it = decomposed_transforms_.find(start_offset);
  if (it == decomposed_transforms_.end()) {
    std::unique_ptr<DecomposedTransform> decomposed_transform =
        std::make_unique<DecomposedTransform>();
    Transform transform = ApplyRemaining(start_offset);
    if (!DecomposeTransform(decomposed_transform.get(), transform)) {
      return false;
    }
    decomposed_transforms_[start_offset] = std::move(decomposed_transform);
  }
  return true;
}

}  // namespace clay
