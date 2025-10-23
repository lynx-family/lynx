// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scrollbar/scrollbar_orientation_helper.h"

#include "clay/ui/component/base_view.h"

namespace clay {

ScrollbarOrientationHelper::ScrollbarOrientationHelper() = default;
ScrollbarOrientationHelper::~ScrollbarOrientationHelper() = default;

float ScrollbarOrientationHelper::GetLocation(const FloatPoint& point) const {
  if (direction_ == ScrollDirection::kVertical) {
    return point.y();
  } else {
    return point.x();
  }
}

float ScrollbarOrientationHelper::GetLength(const FloatSize& delta) const {
  if (direction_ == ScrollDirection::kVertical) {
    return delta.height();
  } else {
    return delta.width();
  }
}

float ScrollbarOrientationHelper::GetLength(const FloatRect& rect) const {
  if (direction_ == ScrollDirection::kVertical) {
    return rect.height();
  } else {
    return rect.width();
  }
}

float ScrollbarOrientationHelper::GetLength(const BaseView& view) const {
  if (direction_ == ScrollDirection::kVertical) {
    return view.Height();
  } else {
    return view.Width();
  }
}

float ScrollbarOrientationHelper::GetSecondaryLength(
    const BaseView& view) const {
  if (direction_ == ScrollDirection::kVertical) {
    return view.Width();
  } else {
    return view.Height();
  }
}

float ScrollbarOrientationHelper::GetContentLength(const BaseView& view) const {
  if (direction_ == ScrollDirection::kVertical) {
    return view.ContentHeight();
  } else {
    return view.ContentWidth();
  }
}

float ScrollbarOrientationHelper::GetSecondaryContentLength(
    const BaseView& view) const {
  if (direction_ == ScrollDirection::kVertical) {
    return view.ContentWidth();
  } else {
    return view.ContentHeight();
  }
}

void ScrollbarOrientationHelper::SetBound(BaseView* view, float location,
                                          float secondary_location,
                                          float length,
                                          float secondary_length) const {
  FML_CHECK(view);
  if (direction_ == ScrollDirection::kVertical) {
    view->SetBound(secondary_location, location, secondary_length, length);
  } else {
    view->SetBound(location, secondary_location, length, secondary_length);
  }
}

}  // namespace clay
