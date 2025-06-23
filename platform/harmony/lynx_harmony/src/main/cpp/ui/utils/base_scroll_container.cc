// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/base_scroll_container.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {
BaseScrollContainer::BaseScrollContainer(LynxContext* context, int sign,
                                         const std::string& tag)
    : UIView(context, ARKUI_NODE_SCROLL, sign, tag) {
  SetNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
  SetScrollbar(false);
  overflow_ = {false, false};
}

void BaseScrollContainer::OnNodeReady() {
  UIBase::OnNodeReady();
  // set nested scroll mode
  if (scroll_forward_mode_ || scroll_backward_mode_) {
    SetNestedScroll(
        scroll_forward_mode_.value_or(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY),
        scroll_backward_mode_.value_or(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY));
  }

  if (IsEnableNewGesture()) {
    SetEnableScrollInteraction(false);
    auto map = UIBase::GetGestureDetectorMap();
    if (!map.empty()) {
      for (const auto& entry : map) {
        if (entry.second->gesture_type() == GestureType::NATIVE) {
          SetEnableScrollInteraction(true);
        }
      }
    }
  }
}

void BaseScrollContainer::SetScrollbar(bool enable_scroll_bar) {
  ArkUI_ScrollBarDisplayMode mode = enable_scroll_bar
                                        ? ARKUI_SCROLL_BAR_DISPLAY_MODE_ON
                                        : ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF;
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_BAR_DISPLAY_MODE, static_cast<int>(mode));
}

void BaseScrollContainer::SetNestedScroll(bool enable_nested_scroll) {
  ArkUI_ScrollNestedMode mode = enable_nested_scroll
                                    ? ARKUI_SCROLL_NESTED_MODE_SELF_FIRST
                                    : ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
  SetNestedScroll(mode, mode);
}

void BaseScrollContainer::SetNestedScroll(
    ArkUI_ScrollNestedMode nested_scroll_forward_mode,
    ArkUI_ScrollNestedMode nested_scroll_backward_mode) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_NESTED_SCROLL,
      static_cast<int>(nested_scroll_forward_mode),
      static_cast<int>(nested_scroll_backward_mode));
}

ArkUI_ScrollNestedMode BaseScrollContainer::GetNestedScrollMode(
    const std::string& str) {
  if (str == "selfOnly") {
    return ArkUI_ScrollNestedMode::ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
  } else if (str == "selfFirst") {
    return ArkUI_ScrollNestedMode::ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
  } else if (str == "parentFirst") {
    return ArkUI_ScrollNestedMode::ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST;
  } else if (str == "parallel") {
    return ArkUI_ScrollNestedMode::ARKUI_SCROLL_NESTED_MODE_PARALLEL;
  }
  return ArkUI_ScrollNestedMode::ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
}

void BaseScrollContainer::SetHorizontal(bool horizontal) {
  is_horizontal_ = horizontal;
  auto scroll_direction =
      horizontal ? ArkUI_ScrollDirection::ARKUI_SCROLL_DIRECTION_HORIZONTAL
                 : ArkUI_ScrollDirection::ARKUI_SCROLL_DIRECTION_VERTICAL;
  SetScrollDirection(scroll_direction);
}

void BaseScrollContainer::SetScrollDirection(ArkUI_ScrollDirection direction) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_SCROLL_DIRECTION, static_cast<int>(direction));
}

void BaseScrollContainer::SetBounces(bool bounces) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_EDGE_EFFECT,
      static_cast<int32_t>(bounces ? ARKUI_EDGE_EFFECT_SPRING
                                   : ARKUI_EDGE_EFFECT_NONE));
}

void BaseScrollContainer::SetEnableScrollInteraction(
    bool enable_scroll_interaction) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_ENABLE_SCROLL_INTERACTION,
      enable_scroll_interaction ? 1 : 0);
}

void BaseScrollContainer::ScrollTo(float x, float y, bool smooth) {
  GestureRecognized();
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_SCROLL_OFFSET, x, y, (smooth ? 250 : 0),
      static_cast<int>(ArkUI_AnimationCurve::ARKUI_CURVE_SMOOTH), 0);
}

float BaseScrollContainer::ScrollX() {
  if (is_horizontal_) {
    auto offset = GetScrollOffset();
    return offset.first;
  }
  return 0;
}

float BaseScrollContainer::ScrollY() {
  if (is_horizontal_) {
    return 0;
  }
  auto offset = GetScrollOffset();
  return offset.second;
}

void BaseScrollContainer::OnPropUpdate(const std::string& name,
                                       const lepus::Value& value) {
  if (name == kNestedScrollForwardOptions && value.IsString()) {
    scroll_forward_mode_ = GetNestedScrollMode(value.StdString());
  } else if (name == kNestedScrollBackWardOptions && value.IsString()) {
    scroll_backward_mode_ = GetNestedScrollMode(value.StdString());
  } else {
    UIView::OnPropUpdate(name, value);
  }
}

bool BaseScrollContainer::IsScrollable() { return true; }

bool BaseScrollContainer::CanConsumeGesture(float deltaX, float deltaY) {
  if (is_horizontal_) {
    if ((IsAtStart() && deltaX < 0) || (IsAtEnd() && deltaX > 0)) {
      return false;
    } else {
      return true;
    }
  } else {
    if ((IsAtStart() && deltaY < 0) || (IsAtEnd() && deltaY > 0)) {
      return false;
    } else {
      return true;
    }
  }
}

bool BaseScrollContainer::IsAtBorder(bool isStart) {
  if (isStart) {
    return IsAtStart();
  } else {
    return IsAtEnd();
  }
}

std::pair<float, float> BaseScrollContainer::GetScrollOffset() const {
  float x{.0f}, y{.0f};
  NodeManager::Instance().GetAttributeValues(node_, NODE_SCROLL_OFFSET, &x, &y);
  return {x, y};
}

float BaseScrollContainer::GetScrollDistance() const {
  auto offset = GetScrollOffset();
  if (is_horizontal_) {
    return offset.first;
  }
  return offset.second;
}

float BaseScrollContainer::GetViewPortSize() const {
  if (IsHorizontal()) {
    return width_ - padding_left_ - padding_right_;
  } else {
    return height_ - padding_bottom_ - padding_top_;
  }
}

bool BaseScrollContainer::IsAtEnd() const {
  auto offset = GetScrollOffset();
  if (is_horizontal_) {
    return static_cast<int64_t>(offset.first + width_ - content_width_) >= 0;
  } else {
    return static_cast<int64_t>(context_->ScaledDensity() *
                                (offset.second + height_ - content_height_)) >=
           0;
  }
}

bool BaseScrollContainer::IsAtStart() const {
  auto offset = GetScrollOffset();
  if (is_horizontal_) {
    return offset.first <= 0;
  } else {
    return offset.second <= 0;
  }
}

bool BaseScrollContainer::IsVerticalScrollView() { return !is_horizontal_; }

// for worklet or gesture
std::vector<float> BaseScrollContainer::ScrollBy(float delta_x, float delta_y) {
  GestureRecognized();
  std::vector<float> res(4, 0);
  float last_scroll_offset = GetScrollDistance();
  if (IsHorizontal()) {
    float scroll_x = last_scroll_offset + delta_x;
    ScrollTo(scroll_x <= 0 ? 0 : scroll_x, 0.f, false);
    res[0] = GetScrollDistance() - last_scroll_offset;
    res[1] = 0;
    res[2] = delta_x - res[0];
    res[3] = delta_y;
  } else {
    float scroll_y = last_scroll_offset + delta_y;
    ScrollTo(0.f, scroll_y <= 0 ? 0 : scroll_y, false);
    res[0] = 0;
    res[1] = GetScrollDistance() - last_scroll_offset;
    res[2] = delta_x;
    res[3] = delta_y - res[1];
  }
  return res;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
