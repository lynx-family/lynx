// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/box_shadow_operations.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/math_util.h"
#include "clay/gfx/style/color.h"

namespace clay {

enum BoxShadowType {
  kNone = 0,
  kInset = 1,
  kInitial,
  kInherit,
};

BoxShadowOperation::BoxShadowOperation(const BoxShadowValue& value)
    : shadow({BoxShadowType::kInset == static_cast<BoxShadowType>(value.option)
                  ? true
                  : false,
              value.h_offset, value.v_offset, value.blur, value.spread,
              value.color}) {}

bool BoxShadowOperation::BlendOperations(const BoxShadowOperation* from,
                                         const BoxShadowOperation* to,
                                         float progress,
                                         BoxShadowOperation* result) {
  std::unique_ptr<BoxShadowOperation> op = nullptr;
  FML_DCHECK(result);
  if (from == nullptr) {
    FML_DCHECK(to);
    op = std::make_unique<BoxShadowOperation>();
    from = op.get();
  } else if (to == nullptr) {
    FML_DCHECK(from);
    op = std::make_unique<BoxShadowOperation>();
    to = op.get();
  }
  FML_DCHECK(from->shadow.inset == to->shadow.inset);
  result->shadow.inset = from->shadow.inset;
  result->shadow.offset_x =
      linearInterpolate(from->shadow.offset_x, to->shadow.offset_x, progress);
  result->shadow.offset_y =
      linearInterpolate(from->shadow.offset_y, to->shadow.offset_y, progress);
  result->shadow.blur_radius = linearInterpolate(
      from->shadow.blur_radius, to->shadow.blur_radius, progress);
  result->shadow.spread_radius = linearInterpolate(
      from->shadow.spread_radius, to->shadow.spread_radius, progress);
  result->shadow.color =
      Color::Lerp(from->shadow.color, to->shadow.color, progress);
  return true;
}

BoxShadowOperations::BoxShadowOperations(
    const std::vector<BoxShadowValue>& values) {
  for (auto& value : values) {
    operations_.emplace_back(value);
  }
}

BoxShadowOperations BoxShadowOperations::Blend(const BoxShadowOperations& other,
                                               float fraction) const {
  BoxShadowOperations to_return;
  if (!BlendInternal(other, fraction, &to_return)) {
    // If the matrices cannot be blended, fallback to discrete animation
    // logic. See
    // https://drafts.csswg.org/css-transforms/#matrix-interpolation
    to_return = fraction < 0.5 ? other : *this;
  }
  return to_return;
}

bool BoxShadowOperations::BlendInternal(const BoxShadowOperations& other,
                                        float progress,
                                        BoxShadowOperations* result) const {
  size_t matching_prefix_length = MatchingPrefixLength(other);
  size_t from_size = other.operations_.size();
  size_t to_size = operations_.size();
  for (size_t i = 0; i < matching_prefix_length; ++i) {
    BoxShadowOperation blended;
    if (!BoxShadowOperation::BlendOperations(
            i >= from_size ? nullptr : &other.operations_[i],
            i >= to_size ? nullptr : &operations_[i], progress, &blended)) {
      return false;
    }
    result->Append(std::move(blended));
  }
  return true;
}

size_t BoxShadowOperations::MatchingPrefixLength(
    const BoxShadowOperations& other) const {
  // If the operations match to the length of the shorter list, then pad its
  // length with the matching identity operations.
  // https://drafts.csswg.org/css-transforms/#transform-function-lists
  return std::max(operations_.size(), other.operations_.size());
}

void BoxShadowOperations::Append(BoxShadowOperation op) {
  operations_.emplace_back(op);
}

}  // namespace clay
