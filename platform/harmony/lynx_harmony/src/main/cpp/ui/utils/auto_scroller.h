// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_AUTO_SCROLLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_AUTO_SCROLLER_H_

#include <arkui/native_node.h>
#include <native_vsync/native_vsync.h>

#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/base_scroll_container.h"
namespace lynx {
namespace tasm {
namespace harmony {

class AutoScroller : public std::enable_shared_from_this<AutoScroller> {
 public:
  explicit AutoScroller(BaseScrollContainer* scroll_container);
  ~AutoScroller();
  void AutoScroll(
      float rate, bool start, bool auto_stop,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  BaseScrollContainer* GetScrollContainer() { return scroll_container_; }
  void AutoScrollInternal();

 private:
  BaseScrollContainer* scroll_container_{nullptr};
  bool block_auto_scroll_{false};
  float rate_{0};
  bool auto_stop_{true};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_AUTO_SCROLLER_H_
