// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATIONS_H_
#define CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATIONS_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "core/animation/transforms/transform_operation.h"
#include "core/renderer/css/css_value.h"
#include "core/style/transform_raw_data.h"

namespace lynx {
namespace tasm {
class Element;
}
namespace transforms {

struct DecomposedTransform;
class Matrix44;

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
  TransformOperations(tasm::Element* element);
  // construct TransformOperations with transform raw data
  TransformOperations(tasm::Element* element, const tasm::CSSValue& raw_data);
  TransformOperations(const TransformOperations& other);
  ~TransformOperations();

  TransformOperations& operator=(const TransformOperations& other);

  static void InitializeTransformOperations(
      TransformOperations& transform_operations,
      std::vector<lynx::starlight::TransformRawData>& transform_raw_data);

  // Returns a transformation matrix representing the set of transform
  // operations from index |start| to the end of the list.
  Matrix44 ApplyRemaining(size_t start);

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
  TransformOperations Blend(TransformOperations& from, float progress);

  // Returns the number of matching transform operations at the start of the
  // transform lists. If one list is shorter but pairwise compatible, it will be
  // extended with matching identity operators per spec
  // (https://drafts.csswg.org/css-transforms/#interpolation-of-transforms).
  size_t MatchingPrefixLength(const TransformOperations& other) const;

  bool IsIdentity() const;
  const std::vector<TransformOperation>& GetOperations() { return operations_; }
  size_t size() const { return operations_.size(); }

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

  void Append(const TransformOperation& operation);
  void AppendDecomposedTransform(const DecomposedTransform& operation);
  void AppendTranslate(starlight::NLength x_value,
                       TransformOperation::LengthType x_type,
                       starlight::NLength y_value,
                       TransformOperation::LengthType y_type,
                       starlight::NLength z_value,
                       TransformOperation::LengthType z_type);
  void AppendTranslateUnitType(lynx::starlight::TransformRawData&);
  void AppendRotate(TransformOperation::Type type, float degree);
  void AppendScale(float x, float y);
  void AppendSkew(float x, float y);
  void AppendMatrix(starlight::TransformType type,
                    const std::array<double, 16>& raw_matrix_data);

  tasm::CSSValue ToTransformRawValue();

 private:
  bool BlendInternal(TransformOperations& from, float progress,
                     TransformOperations* result);

  std::vector<TransformOperation> operations_;

  bool ComputeDecomposedTransform(size_t start_offset);

  // For efficiency, we cache the decomposed transforms.
  mutable std::unordered_map<size_t, std::unique_ptr<DecomposedTransform>>
      decomposed_transforms_;
  tasm::Element* element_{nullptr};
};

}  // namespace transforms
}  // namespace lynx

#endif  // CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATIONS_H_
