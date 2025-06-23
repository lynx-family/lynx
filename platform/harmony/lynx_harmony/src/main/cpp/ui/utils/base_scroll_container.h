// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASE_SCROLL_CONTAINER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASE_SCROLL_CONTAINER_H_

#include <string>
#include <utility>
#include <vector>

#include <arkui/native_type.h>
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr const char* const kNestedScrollForwardOptions = "temporary-nested-scroll-forward";
static constexpr const char* const kNestedScrollBackWardOptions =
    "temporary-nested-scroll-backward";

class BaseScrollContainer : public UIView {
 public:
  void OnNodeReady() override;
  void ScrollTo(float x, float y, bool smooth);
  std::pair<float, float> GetScrollOffset() const;
  float GetScrollDistance() const;
  float GetViewPortSize() const;
  bool IsHorizontal() const { return is_horizontal_; };
  bool IsAtStart() const;
  bool IsAtEnd() const;
  bool CanConsumeGesture(float deltaX, float deltaY) override;
  float ScrollX() override;
  float ScrollY() override;
  bool IsScrollable() override;
  std::vector<float> ScrollBy(float delta_x, float delta_y) override;
  virtual void AutoScrollStopped(){};
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  bool IsAtBorder(bool isStart) override;

 protected:
  BaseScrollContainer(LynxContext* context, int sign, const std::string& tag);
  void SetNestedScroll(bool enable_nested_scroll);
  void SetEnableScrollInteraction(bool enable_scroll_interaction);
  void SetScrollbar(bool enable_scroll_bar);
  void SetHorizontal(bool horizontal);
  void SetBounces(bool bounces);
  virtual void UpdateContentSize(float width, float height) {
    content_width_ = width;
    content_height_ = height;
  };
  bool IsVerticalScrollView() override;
  bool DefaultOverflowValue() override { return false; }

 private:
  void SetNestedScroll(ArkUI_ScrollNestedMode nested_scroll_forward_mode,
                       ArkUI_ScrollNestedMode nested_scroll_backward_mode);
  void SetScrollDirection(ArkUI_ScrollDirection direction);
  ArkUI_ScrollNestedMode GetNestedScrollMode(const std::string& str);

 protected:
  bool is_horizontal_{false};
  float content_width_{0};
  float content_height_{0};

  // “enable-nested-scroll” cannot meet the needs,so add the
  // scrollForward/scrollBackWard options.
  std::optional<ArkUI_ScrollNestedMode> scroll_forward_mode_;
  std::optional<ArkUI_ScrollNestedMode> scroll_backward_mode_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASE_SCROLL_CONTAINER_H_
