// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_punch_hole_layer.h"

namespace clay {

PendingPunchHoleLayer::PendingPunchHoleLayer(const skity::Rect& rect)
    : punch_hole_rect_(rect) {}

PendingPunchHoleLayer::~PendingPunchHoleLayer() = default;

void PendingPunchHoleLayer::AddToFrame(FrameBuilder* builder,
                                       const FloatPoint& offset) {
  builder->AddPunchHole(punch_hole_rect_);
}

#ifndef NDEBUG
std::string PendingPunchHoleLayer::ToString() const {
  std::stringstream ss;
  ss << PendingLayer::ToString();
  ss << " punch_hole_rect=(" << punch_hole_rect_.X() << ", "
     << punch_hole_rect_.Y() << ", " << punch_hole_rect_.Width() << ", "
     << punch_hole_rect_.Height() << ")";
  return ss.str();
}
#endif

}  // namespace clay
