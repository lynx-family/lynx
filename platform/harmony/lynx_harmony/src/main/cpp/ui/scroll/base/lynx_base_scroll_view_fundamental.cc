// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

void LynxBaseScrollView::SetVertical(bool vertical) {
  vertical_ = vertical;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_SCROLL_DIRECTION,
      static_cast<int>(vertical ? ARKUI_SCROLL_DIRECTION_VERTICAL
                                : ARKUI_SCROLL_DIRECTION_HORIZONTAL));
}

bool LynxBaseScrollView::Vertical() { return vertical_; }

void LynxBaseScrollView::EnableScroll(bool enable) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, enable ? 1 : 0);
}

bool LynxBaseScrollView::ScrollEnabled() {
  int enable = 0;
  NodeManager::Instance().GetAttributeValues(
      node_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION, &enable);
  return enable == 1;
}

void LynxBaseScrollView::StopScrolling() {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, x, y, 0,
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
}

void LynxBaseScrollView::SetBounces(bool bounces) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_EDGE_EFFECT,
      static_cast<int32_t>(bounces ? ARKUI_EDGE_EFFECT_SPRING
                                   : ARKUI_EDGE_EFFECT_NONE));
}

bool LynxBaseScrollView::Bounces() {
  int bounces = ARKUI_EDGE_EFFECT_NONE;
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_EDGE_EFFECT,
                                             &bounces);
  return bounces == ARKUI_EDGE_EFFECT_SPRING;
}

void LynxBaseScrollView::EnableScrollBar(bool enable) {
  ArkUI_ScrollBarDisplayMode mode = enable ? ARKUI_SCROLL_BAR_DISPLAY_MODE_ON
                                           : ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_BAR_DISPLAY_MODE, static_cast<int>(mode));
}

bool LynxBaseScrollView::Dragging() { return dragging_; }

int LynxBaseScrollView::CurrentScrollState() {
  return static_cast<int>(scroll_state_);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
