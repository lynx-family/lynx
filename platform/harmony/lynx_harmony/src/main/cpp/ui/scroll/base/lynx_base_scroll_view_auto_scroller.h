// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_SCROLLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_SCROLLER_H_

#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <native_vsync/native_vsync.h>

#include <string>
#include <utility>
#include <vector>

#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxBaseScrollViewAutoScroller
    : public std::enable_shared_from_this<LynxBaseScrollViewAutoScroller> {
 public:
  explicit LynxBaseScrollViewAutoScroller(LynxBaseScrollView* scroll_view);

  void StartAutoScroll(float rate, bool auto_stop);
  void StopAutoScroll();

 private:
  void RequestVsync();
  void Tick();
  LynxBaseScrollView* scroll_view_{nullptr};
  float rate_{0};
  bool auto_stop_{true};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_AUTO_SCROLLER_H_
