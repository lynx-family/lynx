// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_ORIGIN_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_ORIGIN_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/style/length.h"

namespace clay {

class TransformOrigin {
 public:
  TransformOrigin()
      : x_(0.5f, LengthUnit::kPercent), y_(0.5f, LengthUnit::kPercent) {}
  TransformOrigin(float x, float y) : x_(x), y_(y) {}
  FloatPoint GetValue(float width, float height) const {
    return FloatPoint(x_.GetValue(width), y_.GetValue(height));
  }
  void Reset() {
    x_ = Length(0.5f, LengthUnit::kPercent);
    y_ = Length(0.5f, LengthUnit::kPercent);
  }

  void SetX(const Length& x) { x_ = x; }
  void SetY(const Length& y) { y_ = y; }
  void SetX(float v, LengthUnit u) { x_ = Length(v, u); }
  void SetY(float v, LengthUnit u) { y_ = Length(v, u); }

 private:
  Length x_;
  Length y_;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_ORIGIN_H_
