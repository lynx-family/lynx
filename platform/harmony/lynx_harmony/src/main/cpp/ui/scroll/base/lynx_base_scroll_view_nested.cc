// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

NestedScrollMode LynxBaseScrollView::ForwardsNestedScrollMode() {
  return forwards_nested_scroll_mode_;
}

NestedScrollMode LynxBaseScrollView::BackwardsNestedScrollMode() {
  return backwards_nested_scroll_mode_;
}

void LynxBaseScrollView::SetForwardsNestedScrollMode(NestedScrollMode mode) {
  forwards_nested_scroll_mode_ = mode;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_NESTED_SCROLL,
      static_cast<int>(forwards_nested_scroll_mode_),
      static_cast<int>(backwards_nested_scroll_mode_));
}

void LynxBaseScrollView::SetBackwardsNestedScrollMode(NestedScrollMode mode) {
  backwards_nested_scroll_mode_ = mode;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_NESTED_SCROLL,
      static_cast<int>(forwards_nested_scroll_mode_),
      static_cast<int>(backwards_nested_scroll_mode_));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
