// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/gfx_utils.h"

#include <limits>

namespace clay {
void RectBoundsAccumulator::accumulate(const skity::Rect& r, int index) {
  if (r.Left() < r.Right() && r.Top() < r.Bottom()) {
    rect_.accumulate(r.Left(), r.Top());
    rect_.accumulate(r.Right(), r.Bottom());
  }
}

void RectBoundsAccumulator::save() {
  saved_rects_.emplace_back(rect_);
  rect_ = AccumulationRect();
}
void RectBoundsAccumulator::restore() {
  if (!saved_rects_.empty()) {
    skity::Rect layer_bounds = rect_.bounds();
    pop_and_accumulate(layer_bounds, nullptr);
  }
}
bool RectBoundsAccumulator::restore(
    std::function<bool(const skity::Rect&, skity::Rect&)> mapper,
    const skity::Rect* clip) {
  bool success = true;
  if (!saved_rects_.empty()) {
    skity::Rect layer_bounds = rect_.bounds();
    success = mapper(layer_bounds, layer_bounds);
    pop_and_accumulate(layer_bounds, clip);
  }
  return success;
}
void RectBoundsAccumulator::pop_and_accumulate(skity::Rect& layer_bounds,
                                               const skity::Rect* clip) {
  FML_DCHECK(!saved_rects_.empty());

  rect_ = saved_rects_.back();
  saved_rects_.pop_back();

  if (clip == nullptr || layer_bounds.Intersect(*clip)) {
    accumulate(layer_bounds, -1);
  }
}

RectBoundsAccumulator::AccumulationRect::AccumulationRect() {
  min_x_ = std::numeric_limits<float>::infinity();
  min_y_ = std::numeric_limits<float>::infinity();
  max_x_ = -std::numeric_limits<float>::infinity();
  max_y_ = -std::numeric_limits<float>::infinity();
}
void RectBoundsAccumulator::AccumulationRect::accumulate(float x, float y) {
  if (min_x_ > x) {
    min_x_ = x;
  }
  if (min_y_ > y) {
    min_y_ = y;
  }
  if (max_x_ < x) {
    max_x_ = x;
  }
  if (max_y_ < y) {
    max_y_ = y;
  }
}
skity::Rect RectBoundsAccumulator::AccumulationRect::bounds() const {
  return (max_x_ >= min_x_ && max_y_ >= min_y_)
             ? skity::Rect::MakeLTRB(min_x_, min_y_, max_x_, max_y_)
             : skity::Rect::MakeEmpty();
}

}  // namespace clay
