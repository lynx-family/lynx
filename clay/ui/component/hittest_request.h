// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_HITTEST_REQUEST_H_
#define CLAY_UI_COMPONENT_HITTEST_REQUEST_H_

#include "clay/gfx/geometry/float_point.h"

namespace clay {

class HitTestRequest {
 public:
  enum class Type {
    kScroll = 0,
    kClick,
    kTouch,
    kTap,
  };

  HitTestRequest(Type type, const FloatPoint& point)
      : type_(type), point_(point) {}
  ~HitTestRequest() = default;

  Type type() const { return type_; }
  FloatPoint point() const { return point_; }

 private:
  Type type_;
  FloatPoint point_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_HITTEST_REQUEST_H_
