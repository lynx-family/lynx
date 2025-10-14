// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SCROLL_DIRECTION_H_
#define CLAY_GFX_SCROLL_DIRECTION_H_

namespace clay {
// used by both scroll view and list view
enum class ScrollDirection {
  kNone = 0,
  kHorizontal = 1,
  kVertical = kHorizontal << 1,
  // Not support now
  // kBoth = kHorizontal | kVertical,
};

static constexpr ScrollDirection kDefaultScrollDirection =
    ScrollDirection::kVertical;

}  // namespace clay

#endif  // CLAY_GFX_SCROLL_DIRECTION_H_
