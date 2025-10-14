// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/clip_rect_layer.h"

namespace clay {

ClipRectLayer::ClipRectLayer(const skity::Rect& clip_rect, Clip clip_behavior)
    : ClipShapeLayer(clip_rect, clip_behavior) {}

const skity::Rect& ClipRectLayer::clip_shape_bounds() const {
  return clip_shape();
}

void ClipRectLayer::ApplyClip(LayerStateStack::MutatorContext& mutator) const {
  mutator.clipRect(clip_shape(), clip_behavior() != Clip::hardEdge);
}

#ifndef NDEBUG
std::string ClipRectLayer::ToString() const {
  std::stringstream ss;
  ss << ContainerLayer::ToString();
  auto clip_rect = clip_shape_bounds();
  ss << " clip_rect_=(" << clip_rect.Left() << "," << clip_rect.Top() << ","
     << clip_rect.Width() << "," << clip_rect.Height() << ")";
  ss << " clip_behavior_=" << clip_behavior();
  return ss.str();
}
#endif

}  // namespace clay
