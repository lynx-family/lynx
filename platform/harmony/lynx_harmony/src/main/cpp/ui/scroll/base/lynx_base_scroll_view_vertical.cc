// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

float LynxBaseScrollView::GetScrollOffsetVertically() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  return y;
}

void LynxBaseScrollView::SetScrollContentSizeVertically(float content_size) {
  content_size_[1] = content_size;
}

void LynxBaseScrollView::ScrollByUnlimitedVertically(float delta) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  ScrollToUnlimitedVertically(y + delta);
}

void LynxBaseScrollView::ScrollByVertically(float delta) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  ScrollToVertically(y + delta);
}

void LynxBaseScrollView::ScrollToUnlimitedVertically(float offset) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, x, offset, 0,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
}

void LynxBaseScrollView::ScrollToVertically(float offset) {
  float range[2] = {.0f, .0f};
  GetScrollRangeVertically(range);
  offset = std::min(std::max(offset, range[0]), range[1]);
  ScrollToUnlimitedVertically(offset);
}

void LynxBaseScrollView::AnimatedScrollToVertically(float offset) {
  float range[2] = {.0f, .0f};
  GetScrollRangeVertically(range);
  offset = std::min(std::max(offset, range[0]), range[1]);
  AnimatedScrollToUnlimitedVertically(offset);
}

void LynxBaseScrollView::AnimatedScrollToUnlimitedVertically(float offset) {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, x, offset, 300,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
  TryToUpdateScrollState(
      LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateAnimating);
}

void LynxBaseScrollView::GetScrollRangeVertically(float range[2]) {
  float height{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_HEIGHT, &height);
  range[0] = 0.0f;
  range[1] = content_size_[1] - height;
}

bool LynxBaseScrollView::CanScrollForwardsVertically() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  float range[2] = {.0f, .0f};
  GetScrollRangeVertically(range);
  if (y >= range[1]) {
    return false;
  } else {
    return true;
  }
}

bool LynxBaseScrollView::CanScrollBackwardsVertically() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  float range[2] = {.0f, .0f};
  GetScrollRangeVertically(range);
  if (y <= range[0]) {
    return false;
  } else {
    return true;
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
