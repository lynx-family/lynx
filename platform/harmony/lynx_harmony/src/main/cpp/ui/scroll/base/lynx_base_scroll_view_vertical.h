// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_VERTICAL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_VERTICAL_H_

#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxBaseScrollViewVertical {
 public:
  virtual float GetScrollOffsetVertically() = 0;

  virtual void SetScrollContentSizeVertically(float content_size) = 0;

  virtual void ScrollByUnlimitedVertically(float delta) = 0;

  virtual void ScrollByVertically(float delta) = 0;

  virtual void ScrollToUnlimitedVertically(float offset) = 0;

  virtual void ScrollToVertically(float offset) = 0;

  virtual void AnimatedScrollToVertically(float offset) = 0;

  virtual void AnimatedScrollToUnlimitedVertically(float offset) = 0;

  virtual void GetScrollRangeVertically(float range[2]) = 0;

  virtual bool CanScrollForwardsVertically() = 0;

  virtual bool CanScrollBackwardsVertically() = 0;

  virtual ~LynxBaseScrollViewVertical() = default;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_VERTICAL_H_
