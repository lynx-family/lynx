// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

float LynxBaseScrollView::GetScrollOffsetHorizontally() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  return x;
}

void LynxBaseScrollView::SetScrollContentSizeHorizontally(float content_size) {
  content_size_[0] = content_size;
}

void LynxBaseScrollView::ScrollByUnlimitedHorizontally(float delta) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  ScrollToUnlimitedHorizontally(x + delta);
}

void LynxBaseScrollView::ScrollByHorizontally(float delta) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  ScrollToHorizontally(x + delta);
}

void LynxBaseScrollView::ScrollToUnlimitedHorizontally(float offset) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, offset, y, 0,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
}

void LynxBaseScrollView::ScrollToHorizontally(float offset) {
  float range[2] = {.0f, .0f};
  GetScrollRangeHorizontally(range);
  offset = std::min(std::max(offset, range[0]), range[1]);
  ScrollToUnlimitedHorizontally(offset);
}

void LynxBaseScrollView::AnimatedScrollToHorizontally(float offset) {
  float range[2] = {.0f, .0f};
  GetScrollRangeHorizontally(range);
  offset = std::min(std::max(offset, range[0]), range[1]);
  AnimatedScrollToUnlimitedHorizontally(offset);
}

void LynxBaseScrollView::AnimatedScrollToUnlimitedHorizontally(float offset) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, offset, y, 300,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
  TryToUpdateScrollState(
      LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateAnimating);
}

void LynxBaseScrollView::GetScrollRangeHorizontally(float range[2]) {
  float width{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_WIDTH, &width);
  range[0] = 0.0f;
  range[1] = content_size_[0] - width;
}

bool LynxBaseScrollView::CanScrollForwardsHorizontally() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  float range[2] = {.0f, .0f};
  GetScrollRangeHorizontally(range);
  if (x >= range[1]) {
    return false;
  } else {
    return true;
  }
}

bool LynxBaseScrollView::CanScrollBackwardsHorizontally() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  float range[2] = {.0f, .0f};
  GetScrollRangeHorizontally(range);
  if (x <= range[0]) {
    return false;
  } else {
    return true;
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
