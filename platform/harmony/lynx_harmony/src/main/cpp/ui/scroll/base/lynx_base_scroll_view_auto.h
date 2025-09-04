// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_H_

#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxBaseScrollViewAuto {
 public:
  virtual void GetScrollOffset(float scroll_offset[2]) = 0;

  virtual void SetScrollContentSize(float content_size[2]) = 0;

  virtual void ScrollByUnlimited(float delta[2]) = 0;

  virtual void ScrollBy(float delta[2]) = 0;

  virtual void ScrollToUnlimited(float offset[2]) = 0;

  virtual void ScrollTo(float offset[2]) = 0;

  virtual void AnimatedScrollTo(float offset[2]) = 0;

  virtual void AnimatedScrollToUnlimited(float offset[2]) = 0;

  virtual void GetScrollRange(float range[4]) = 0;

  virtual bool CanScrollForwards() = 0;

  virtual bool CanScrollBackwards() = 0;

  virtual ~LynxBaseScrollViewAuto() = default;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_H_
