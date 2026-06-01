// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/frame_shadow_node.h"

#include <algorithm>
#include <cmath>

#include "base/include/float_comparison.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

float MeasureDimension(float size, MeasureMode mode, float intrinsic_size) {
  if (mode == MeasureMode::Definite) {
    return std::max(0.f, size);
  }
  if (base::FloatsLarger(intrinsic_size, 0.f)) {
    if (mode == MeasureMode::AtMost) {
      return std::min(std::max(0.f, size), intrinsic_size);
    }
    return intrinsic_size;
  }
  if (mode == MeasureMode::AtMost) {
    return std::max(0.f, size);
  }
  return 0.f;
}

}  // namespace

FrameShadowNode::FrameShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

LayoutResult FrameShadowNode::Measure(float width, MeasureMode width_mode,
                                      float height, MeasureMode height_mode,
                                      bool final_measure) {
  return LayoutResult(MeasureDimension(width, width_mode, intrinsic_width_),
                      MeasureDimension(height, height_mode, intrinsic_height_),
                      0);
}

void FrameShadowNode::UpdateIntrinsicSize(float intrinsic_width,
                                          float intrinsic_height) {
  intrinsic_width = std::max(0.f, intrinsic_width);
  intrinsic_height = std::max(0.f, intrinsic_height);
  if (std::abs(intrinsic_width_ - intrinsic_width) < 0.5f &&
      std::abs(intrinsic_height_ - intrinsic_height) < 0.5f) {
    return;
  }
  intrinsic_width_ = intrinsic_width;
  intrinsic_height_ = intrinsic_height;
  MarkDirty();
  RequestLayout();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
