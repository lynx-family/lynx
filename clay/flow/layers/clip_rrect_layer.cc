// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/clip_rrect_layer.h"

namespace clay {

ClipRRectLayer::ClipRRectLayer(const skity::RRect& clip_rrect,
                               Clip clip_behavior)
    : ClipShapeLayer(clip_rrect, clip_behavior) {}

const skity::Rect& ClipRRectLayer::clip_shape_bounds() const {
  return clip_shape().GetBounds();
}

void ClipRRectLayer::ApplyClip(LayerStateStack::MutatorContext& mutator) const {
  mutator.clipRRect(clip_shape(), clip_behavior() != Clip::hardEdge);
}

}  // namespace clay
