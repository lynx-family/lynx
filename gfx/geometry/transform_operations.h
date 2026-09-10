// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_
#define GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_

#include <array>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "gfx/geometry/decomposed_transform.h"
#include "gfx/geometry/transform_operation.h"
#include "gfx/gfx_export.h"

namespace lynx {
namespace gfx {

class GFX_EXPORT TransformOperations {
 public:
  constexpr static float kApproximatelyEqualTolerance = 1e-3f;

  TransformOperations() = default;
  TransformOperations(const TransformOperations& other);
  TransformOperations(TransformOperations&& other) noexcept = default;
  ~TransformOperations() = default;

  TransformOperations& operator=(const TransformOperations& other);
  TransformOperations& operator=(TransformOperations&& other) noexcept =
      default;

  Matrix44 ApplyRemaining(size_t start, float reference_width,
                          float reference_height) const;

  TransformOperations Blend(const TransformOperations& from, float progress,
                            float reference_width,
                            float reference_height) const;
  bool TryBlend(const TransformOperations& from, float progress,
                float reference_width, float reference_height,
                TransformOperations* result) const;

  size_t MatchingPrefixLength(const TransformOperations& other) const;
  bool IsIdentity() const;
  bool ApproximatelyEqual(const TransformOperations& other,
                          float tolerance) const;

  const std::vector<TransformOperation>& GetOperations() const {
    return operations_;
  }
  size_t size() const { return operations_.size(); }

  void Append(const TransformOperation& operation);
  void AppendTranslate(LengthValue x, LengthValue y, LengthValue z);
  void AppendRotate(TransformOperation::Type type, float degree);
  void AppendScale(float x, float y);
  void AppendSkew(float x, float y);
  void AppendMatrix(TransformOperation::Type type,
                    const std::array<double, 16>& matrix);
  void AppendMatrix(TransformOperation::Type type,
                    const std::array<float, 16>& matrix);

 private:
  bool BlendInternal(const TransformOperations& from, float progress,
                     float reference_width, float reference_height,
                     TransformOperations* result) const;
  bool ComputeDecomposedTransform(size_t start_offset, float reference_width,
                                  float reference_height) const;

  std::vector<TransformOperation> operations_;
  mutable std::unordered_map<size_t, DecomposedTransform> decomposed_cache_;
  mutable float decomposed_cache_reference_width_{0.0f};
  mutable float decomposed_cache_reference_height_{0.0f};
  mutable bool has_decomposed_cache_reference_size_{false};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_
