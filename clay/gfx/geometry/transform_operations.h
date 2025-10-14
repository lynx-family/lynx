// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_

#include <cmath>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/gfx/geometry/transform_operation.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "clay/public/style_types.h"

namespace clay {

struct DecomposedTransform;

// Transform operations are a decomposed transformation matrix. It can be
// applied to obtain a Transform at any time, and can be blended
// intelligently with other transform operations, so long as they represent the
// same decomposition. For example, if we have a transform that is made up of
// a rotation followed by skew, it can be blended intelligently with another
// transform made up of a rotation followed by a skew. Blending is possible if
// we have two dissimilar sets of transform operations, but the effect may not
// be what was intended. For more information, see the comments for the blend
// function below.
class TransformOperations {
 public:
  constexpr static float kApproximatelyEqualTolerance = 1e-3;

  TransformOperations();
  TransformOperations(const TransformOperations& other);
  ~TransformOperations();

  TransformOperations& operator=(const TransformOperations& other);

  explicit TransformOperations(const ClayTransform& rk_transform,
                               FloatSize percentage_resolution_size);

  explicit TransformOperations(const std::vector<TransformRaw>& transform_raws,
                               float width, float height);

  // Returns a transformation matrix representing these transform operations.
  Transform Apply() const;

  // Returns a transformation matrix representing the set of transform
  // operations from index |start| to the end of the list.
  Transform ApplyRemaining(size_t start) const;

  // Given another set of transform operations and a progress in the range
  // [0, 1], returns a transformation matrix representing the intermediate
  // value. If this->MatchesTypes(from), then each of the operations are
  // blended separately and then combined. Otherwise, the two sets of
  // transforms are baked to matrices (using apply), and the matrices are
  // then decomposed and interpolated. For more information, see
  // http://www.w3.org/TR/2011/WD-css3-2d-transforms-20111215/#matrix-decomposition.
  //
  // If either of the matrices are non-decomposable for the blend, Blend applies
  // discrete interpolation between them based on the progress value.
  TransformOperations Blend(const TransformOperations& from,
                            float progress) const;

  // Returns true if these operations are only translations.
  bool IsTranslation() const;

  // Returns false if the operations affect 2d axis alignment.
  bool PreservesAxisAlignment() const;

  // Returns true if this operation and its descendants have the same types
  // as other and its descendants.
  bool MatchesTypes(const TransformOperations& other) const;

  // Returns the number of matching transform operations at the start of the
  // transform lists. If one list is shorter but pairwise compatible, it will be
  // extended with matching identity operators per spec
  // (https://drafts.csswg.org/css-transforms/#interpolation-of-transforms).
  size_t MatchingPrefixLength(const TransformOperations& other) const;

  // Returns true if these operations can be blended. It will only return
  // false if we must resort to matrix interpolation, and matrix interpolation
  // fails (this can happen if either matrix cannot be decomposed).
  bool CanBlendWith(const TransformOperations& other) const;

  // If none of these operations have a perspective component, sets |scale| to
  // be the product of the scale component of every operation. Otherwise,
  // returns false.
  bool ScaleComponent(float* scale) const;

  void AppendTranslate(float x, float y, float z);
  void AppendRotate(float x, float y, float z, float degrees);
  void AppendScale(float x, float y, float z);
  void AppendSkewX(float x);
  void AppendSkewY(float y);
  void AppendSkew(float x, float y);
  void AppendPerspective(float depth);
  void AppendMatrix(const double matrix[16]);
  void AppendMatrix(const Transform& matrix);
  void AppendIdentity();
  void Append(const TransformOperation& operation);
  void Append(const TransformOperations& ops);
  bool IsIdentity() const;

  size_t size() const { return operations_.size(); }

  const TransformOperation& at(size_t index) const {
    FML_DCHECK(index < size());
    return operations_[index];
  }
  TransformOperation& at(size_t index) {
    FML_DCHECK(index < size());
    return operations_[index];
  }

  bool ApproximatelyEqual(const TransformOperations& other,
                          float tolerance) const;

  float GetTranslateZ() const {
    float result = 0.f;
    for (auto& operation : operations_) {
      if (operation.type == TransformOperation::kTypeTranslate &&
          std::fabs(operation.translate.z) > 1e-6) {
        result += operation.translate.z;
      }
    }
    return result;
  }

  void SetPerspective(float perspective) { perspective_ = perspective; }

 private:
  void ApplyPerspective() const;
  bool BlendInternal(const TransformOperations& from, float progress,
                     TransformOperations* result) const;

  mutable std::deque<TransformOperation> operations_;

  bool ComputeDecomposedTransform(size_t start_offset) const;

  // For efficiency, we cache the decomposed transforms.
  mutable std::unordered_map<size_t, std::unique_ptr<DecomposedTransform>>
      decomposed_transforms_;

  float perspective_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_H_
