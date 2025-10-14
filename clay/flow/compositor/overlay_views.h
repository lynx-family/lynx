// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_OVERLAY_VIEWS_H_
#define CLAY_FLOW_COMPOSITOR_OVERLAY_VIEWS_H_

#include "skity/geometry/rect.hpp"

namespace clay {

class OverlayViewParams {
 public:
  OverlayViewParams() = default;

  explicit OverlayViewParams(skity::Rect bounds) : bounds_(bounds) {}

  const skity::Rect& bounds() const { return bounds_; }

 private:
  skity::Rect bounds_;
};

}  // namespace clay

#endif  // CLAY_FLOW_COMPOSITOR_OVERLAY_VIEWS_H_
