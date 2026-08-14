// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#endif

#include "gfx/geometry/transform_operations.h"

#include <algorithm>
#include <cmath>

namespace lynx {
namespace gfx {

TransformOperations::TransformOperations(const TransformOperations& other)
    : operations_(other.operations_) {}

TransformOperations& TransformOperations::operator=(
    const TransformOperations& other) {
  if (this != &other) {
    operations_ = other.operations_;
    decomposed_cache_.clear();
    has_decomposed_cache_reference_size_ = false;
  }
  return *this;
}

Matrix44 TransformOperations::ApplyRemaining(size_t start,
                                             float reference_width,
                                             float reference_height) const {
  Matrix44 result;
  for (size_t i = start; i < operations_.size(); ++i) {
    result.preConcat(
        operations_[i].GetMatrix(reference_width, reference_height));
  }
  return result;
}

TransformOperations TransformOperations::Blend(const TransformOperations& from,
                                               float progress,
                                               float reference_width,
                                               float reference_height) const {
  TransformOperations result;
  if (!TryBlend(from, progress, reference_width, reference_height, &result)) {
    return progress < 0.5f ? from : *this;
  }
  return result;
}

bool TransformOperations::TryBlend(const TransformOperations& from,
                                   float progress, float reference_width,
                                   float reference_height,
                                   TransformOperations* result) const {
  if (!result) {
    return false;
  }
  TransformOperations blended;
  if (!BlendInternal(from, progress, reference_width, reference_height,
                     &blended)) {
    return false;
  }
  *result = blended;
  return true;
}

size_t TransformOperations::MatchingPrefixLength(
    const TransformOperations& other) const {
  const size_t operation_count =
      std::min(operations_.size(), other.operations_.size());
  for (size_t i = 0; i < operation_count; ++i) {
    if (operations_[i].type != other.operations_[i].type ||
        operations_[i].type == TransformOperation::kMatrix ||
        operations_[i].type == TransformOperation::kMatrix3d) {
      return i;
    }
  }
  return std::max(operations_.size(), other.operations_.size());
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
  if (operations_.size() != other.operations_.size()) {
    return false;
  }
  for (size_t i = 0; i < operations_.size(); ++i) {
    if (!operations_[i].ApproximatelyEqual(other.operations_[i], tolerance)) {
      return false;
    }
  }
  return true;
}

void TransformOperations::Append(const TransformOperation& operation) {
  operations_.push_back(operation);
  decomposed_cache_.clear();
}

void TransformOperations::AppendTranslate(LengthValue x, LengthValue y,
                                          LengthValue z) {
  TransformOperation operation;
  operation.type = TransformOperation::kTranslate;
  operation.translate.x = x;
  operation.translate.y = y;
  operation.translate.z = z;
  Append(operation);
}

void TransformOperations::AppendRotate(TransformOperation::Type type,
                                       float degree) {
  TransformOperation operation;
  operation.type = type;
  operation.rotate.degree = degree;
  Append(operation);
}

void TransformOperations::AppendScale(float x, float y) {
  TransformOperation operation;
  operation.type = TransformOperation::kScale;
  operation.scale.x = x;
  operation.scale.y = y;
  Append(operation);
}

void TransformOperations::AppendSkew(float x, float y) {
  TransformOperation operation;
  operation.type = TransformOperation::kSkew;
  operation.skew.x = x;
  operation.skew.y = y;
  Append(operation);
}

void TransformOperations::AppendMatrix(
    TransformOperation::Type type,
    const std::array<double, 16>& raw_matrix_data) {
  TransformOperation operation;
  operation.type = type;
  std::transform(raw_matrix_data.begin(), raw_matrix_data.end(),
                 operation.matrix.matrix_data.begin(),
                 [](double value) { return static_cast<float>(value); });
  Append(operation);
}

void TransformOperations::AppendMatrix(
    TransformOperation::Type type,
    const std::array<float, 16>& raw_matrix_data) {
  TransformOperation operation;
  operation.type = type;
  operation.matrix.matrix_data = raw_matrix_data;
  Append(operation);
}

bool TransformOperations::BlendInternal(const TransformOperations& from,
                                        float progress, float reference_width,
                                        float reference_height,
                                        TransformOperations* result) const {
  const bool from_identity = from.IsIdentity();
  const bool to_identity = IsIdentity();
  if (from_identity && to_identity) {
    return true;
  }

  const size_t matching_prefix_length = MatchingPrefixLength(from);
  const size_t from_size = from_identity ? 0 : from.operations_.size();
  const size_t to_size = to_identity ? 0 : operations_.size();
  const size_t operation_count = std::max(from_size, to_size);

  for (size_t i = 0; i < matching_prefix_length; ++i) {
    TransformOperation operation;
    if (!TransformOperation::BlendTransformOperations(
            i >= from_size ? nullptr : &from.operations_[i],
            i >= to_size ? nullptr : &operations_[i], progress, reference_width,
            reference_height, &operation)) {
      return false;
    }
    result->Append(operation);
  }

  if (matching_prefix_length < operation_count) {
    if (!ComputeDecomposedTransform(matching_prefix_length, reference_width,
                                    reference_height) ||
        !from.ComputeDecomposedTransform(matching_prefix_length,
                                         reference_width, reference_height)) {
      return false;
    }
    result->Append(
        TransformOperation::FromDecomposedTransform(BlendDecomposedTransforms(
            decomposed_cache_.at(matching_prefix_length),
            from.decomposed_cache_.at(matching_prefix_length), progress)));
  }
  return true;
}

bool TransformOperations::ComputeDecomposedTransform(
    size_t start_offset, float reference_width, float reference_height) const {
  if (!has_decomposed_cache_reference_size_ ||
      decomposed_cache_reference_width_ != reference_width ||
      decomposed_cache_reference_height_ != reference_height) {
    decomposed_cache_.clear();
    decomposed_cache_reference_width_ = reference_width;
    decomposed_cache_reference_height_ = reference_height;
    has_decomposed_cache_reference_size_ = true;
  }

  if (decomposed_cache_.find(start_offset) != decomposed_cache_.end()) {
    return true;
  }

  DecomposedTransform transform;
  if (!DecomposeTransform(
          &transform,
          ApplyRemaining(start_offset, reference_width, reference_height))) {
    return false;
  }
  decomposed_cache_.insert_or_assign(start_offset, transform);
  return true;
}

}  // namespace gfx
}  // namespace lynx
