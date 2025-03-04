// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_DIRECTION_SELECTOR_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_DIRECTION_SELECTOR_H_

#include "core/renderer/starlight/layout/box_info.h"

namespace lynx {
namespace starlight {

class DirectionSelector {
 public:
  explicit DirectionSelector(bool is_row, bool is_reverse, bool is_any_rtl) {
    is_horizontal_ = is_row;

    if (is_row) {
      if ((is_any_rtl && !is_reverse) || (!is_any_rtl && is_reverse)) {
        kMainFront = Direction::kRight;
        kMainBack = Direction::kLeft;
      } else {
        kMainFront = Direction::kLeft;
        kMainBack = Direction::kRight;
      }
      kCrossFront = Direction::kTop;
      kCrossBack = Direction::kBottom;
      kMainAxis = Dimension::kHorizontal;
      kCrossAxis = Dimension::kVertical;
    } else {
      if (is_any_rtl) {
        kCrossFront = Direction::kRight;
        kCrossBack = Direction::kLeft;
      } else {
        kCrossFront = Direction::kLeft;
        kCrossBack = Direction::kRight;
      }
      if (is_reverse) {
        kMainFront = Direction::kBottom;
        kMainBack = Direction::kTop;
      } else {
        kMainFront = Direction::kTop;
        kMainBack = Direction::kBottom;
      }

      kMainAxis = Dimension::kVertical;
      kCrossAxis = Dimension::kHorizontal;
    }
  }
  Direction MainFront() const { return Direction(kMainFront); }
  Direction MainBack() const { return Direction(kMainBack); }
  Direction CrossFront() const { return Direction(kCrossFront); }
  Direction CrossBack() const { return Direction(kCrossBack); }
  Dimension MainAxis() const { return Dimension(kMainAxis); }
  Dimension CrossAxis() const { return Dimension(kCrossAxis); }

  Direction HorizontalFront() const {
    return is_horizontal_ ? MainFront() : CrossFront();
  }
  Direction HorizontalBack() const {
    return is_horizontal_ ? MainBack() : CrossBack();
  }
  Direction VerticalFront() const {
    return is_horizontal_ ? CrossFront() : MainFront();
  }
  Direction VerticalBack() const {
    return is_horizontal_ ? CrossBack() : MainBack();
  }

  bool IsHorizontal() const { return is_horizontal_; }

 protected:
  // TODO(zhixuan): move below to private and access them through const
  // accessors.
  unsigned kMainFront : 2;
  unsigned kMainBack : 2;
  unsigned kCrossFront : 2;
  unsigned kCrossBack : 2;
  unsigned kMainAxis : 1;
  unsigned kCrossAxis : 1;

 private:
  bool is_horizontal_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_DIRECTION_SELECTOR_H_
