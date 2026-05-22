// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_point.h"

#include "clay/gfx/geometry/point.h"

namespace clay {

FloatPoint::FloatPoint(const Point& point) : x_(point.x()), y_(point.y()) {}

}  // namespace clay
