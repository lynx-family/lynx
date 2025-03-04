// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_RESULT_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_RESULT_H_

#include "base/include/geometry/point.h"
#include "base/include/geometry/size.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_directions.h"

namespace lynx {
namespace starlight {

using FloatPoint = base::geometry::FloatPoint;
struct LayoutResultForRendering {
  DirectionValue<float> padding_, border_, margin_, sticky_pos_;
  FloatPoint offset_;
  FloatSize size_;
  LayoutResultForRendering()
      : padding_{0, 0, 0, 0},
        border_{0, 0, 0, 0},
        margin_{0, 0, 0, 0},
        sticky_pos_{0, 0, 0, 0},
        offset_(0, 0),
        size_{0.f, 0.f} {}
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_RESULT_H_
