// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/clip_path_layer.h"

namespace clay {

ClipPathLayer::ClipPathLayer(const clay::GrPath& clip_path, Clip clip_behavior)
    : ClipShapeLayer(clip_path, clip_behavior) {
  RECT_ASSIGN(clip_shape_bounds_, PATH_GET_BOUNDS(clip_shape()));
}

const skity::Rect& ClipPathLayer::clip_shape_bounds() const {
  return clip_shape_bounds_;
}

void ClipPathLayer::ApplyClip(LayerStateStack::MutatorContext& mutator) const {
  mutator.clipPath(clip_shape(), clip_behavior() != Clip::hardEdge);
}

}  // namespace clay
