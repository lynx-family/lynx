// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_HORIZONTAL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_HORIZONTAL_H_

#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxBaseScrollViewHorizontal {
 public:
  virtual float GetScrollOffsetHorizontally() = 0;

  virtual void SetScrollContentSizeHorizontally(float content_size) = 0;

  virtual void ScrollByUnlimitedHorizontally(float delta) = 0;

  virtual void ScrollByHorizontally(float delta) = 0;

  virtual void ScrollToUnlimitedHorizontally(float offset) = 0;

  virtual void ScrollToHorizontally(float offset) = 0;

  virtual void AnimatedScrollToHorizontally(float offset) = 0;

  virtual void AnimatedScrollToUnlimitedHorizontally(float offset) = 0;

  virtual void GetScrollRangeHorizontally(float range[2]) = 0;

  virtual bool CanScrollForwardsHorizontally() = 0;

  virtual bool CanScrollBackwardsHorizontally() = 0;

  virtual ~LynxBaseScrollViewHorizontal() = default;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_HORIZONTAL_H_
