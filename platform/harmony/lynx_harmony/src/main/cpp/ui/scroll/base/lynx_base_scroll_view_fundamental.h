// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_FUNDAMENTAL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_FUNDAMENTAL_H_

#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxBaseScrollViewFundamental {
 public:
  virtual void SetVertical(bool vertical) = 0;

  virtual bool Vertical() = 0;

  virtual void EnableScroll(bool enable) = 0;

  virtual bool ScrollEnabled() = 0;

  virtual void StopScrolling() = 0;

  virtual void SetBounces(bool bounces) = 0;

  virtual bool Bounces() = 0;

  virtual void EnableScrollBar(bool enable) = 0;

  virtual bool Dragging() = 0;

  virtual int CurrentScrollState() = 0;

  virtual ~LynxBaseScrollViewFundamental() = default;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_FUNDAMENTAL_H_
