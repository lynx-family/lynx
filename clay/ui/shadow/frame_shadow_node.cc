// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/frame_shadow_node.h"

#include <algorithm>

namespace clay {

FrameShadowNode::FrameShadowNode(ShadowNodeOwner* owner, std::string tag,
                                 int id)
    : ShadowNode(owner, std::move(tag), id) {}

MeasureResult FrameShadowNode::Measure(const MeasureConstraint& constraint) {
  std::scoped_lock lock(mutex_);
  return {ResolveDimension(intrinsic_width_, constraint.width,
                           constraint.width_mode),
          ResolveDimension(intrinsic_height_, constraint.height,
                           constraint.height_mode),
          0.f};
}

void FrameShadowNode::SetIntrinsicContentSize(std::optional<float> width,
                                              std::optional<float> height) {
  bool changed = false;
  {
    std::scoped_lock lock(mutex_);
    if (intrinsic_width_ != width || intrinsic_height_ != height) {
      intrinsic_width_ = width;
      intrinsic_height_ = height;
      changed = true;
    }
  }
  if (changed) {
    MarkDirty();
  }
}

float FrameShadowNode::ResolveDimension(std::optional<float> intrinsic,
                                        std::optional<float> constraint,
                                        MeasureMode mode) {
  if (!intrinsic) {
    return constraint.value_or(0.f);
  }

  switch (mode) {
    case MeasureMode::kDefinite:
      return constraint.value_or(*intrinsic);
    case MeasureMode::kAtMost:
      return constraint ? std::min(*intrinsic, *constraint) : *intrinsic;
    case MeasureMode::kIndefinite:
    default:
      return *intrinsic;
  }
}

}  // namespace clay
