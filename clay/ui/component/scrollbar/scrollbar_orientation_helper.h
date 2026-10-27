// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_ORIENTATION_HELPER_H_
#define CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_ORIENTATION_HELPER_H_

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/scroll_direction.h"

namespace clay {

class BaseView;

class ScrollbarOrientationHelper {
 public:
  ScrollbarOrientationHelper();
  ~ScrollbarOrientationHelper();

  void SetDirection(ScrollDirection direction) { direction_ = direction; }
  ScrollDirection GetDirection() const { return direction_; }

  // Return x() or y() of `point` according to the direction.
  float GetLocation(const FloatPoint& point) const;
  // Return width() or height() of `delta` according to the direction.
  float GetLength(const FloatSize& delta) const;
  float GetLength(const FloatRect& rect) const;
  // Return Width() or Height() of `view` according to the direction.
  float GetLength(const BaseView& view) const;
  float GetSecondaryLength(const BaseView& view) const;

  // Return ContentWidth() or ContentHeight() of `view` according to the
  // direction.
  float GetContentLength(const BaseView& view) const;
  float GetSecondaryContentLength(const BaseView& view) const;

  void SetBound(BaseView* view, float location, float secondary_location,
                float length, float secondary_length) const;

 private:
  friend class ScrollbarView;

  ScrollDirection direction_ = ScrollDirection::kVertical;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLBAR_SCROLLBAR_ORIENTATION_HELPER_H_
