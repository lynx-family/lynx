// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/auto_scroller.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/base_scroll_container.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace scroll {
// props
static constexpr const char* const kScrollX = "scroll-x";
static constexpr const char* const kScrollY = "scroll-y";
static constexpr const char* const kEnableScroll = "enable-scroll";
static constexpr const char* const kEnableNestedScroll = "enable-nested-scroll";
static constexpr const char* const kBounces = "bounces";
static constexpr const char* const kLowerThreshold = "lower-threshold";
static constexpr const char* const kUpperThreshold = "upper-threshold";
static constexpr const char* const kScrollToIndex = "scroll-to-index";
static constexpr const char* const kScrollLeft = "scroll-left";
static constexpr const char* const kScrollTop = "scroll-top";
static constexpr const char* const kEnableScrollBar = "scroll-bar-enable";
// event
static constexpr const char* const kScrollEvent = "scroll";
static constexpr const char* const kScrollUpperEvent = "scrolltoupper";
static constexpr const char* const kScrollLowerEvent = "scrolltolower";
static constexpr const char* const kScrollToUpperEdge = "scrolltoupperedge";
static constexpr const char* const kScrollToLowerEdge = "scrolltoloweredge";
static constexpr const char* const kScrollToNormalState = "scrolltonormalstate";
static constexpr const char* const kScrollStartEvent = "scrollstart";
static constexpr const char* const kScrollEndEvent = "scrollend";
static constexpr const char* const kContentSizeChangeEvent =
    "contentsizechanged";
// event_type

static constexpr ArkUI_NodeEventType kScrollNodeEventTypes[] = {
    NODE_SCROLL_EVENT_ON_SCROLL,      NODE_SCROLL_EVENT_ON_SCROLL_START,
    NODE_SCROLL_EVENT_ON_SCROLL_STOP, NODE_SCROLL_EVENT_ON_SCROLL_EDGE,
    NODE_SCROLL_EVENT_ON_WILL_SCROLL,
};
// AlignmentOptions
static constexpr const char* const kNearest = "nearest";
static constexpr const char* const kCenter = "center";
static constexpr const char* const kEnd = "end";
static constexpr const char* const kStart = "start";

}  // namespace scroll

class UIScroll : public BaseScrollContainer {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScroll(context, sign, tag);
  }

  ~UIScroll() override;

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;

  void SetEvents(const std::vector<lepus::Value>& events) override;

  void AddChild(lynx::tasm::harmony::UIBase* child, int index) override;

  void RemoveChild(lynx::tasm::harmony::UIBase* child) override;

  void OnMeasure(ArkUI_LayoutConstraint* layout_constraint) override;

  void OnNodeEvent(ArkUI_NodeEvent* event) override;

  void OnNodeReady() override;

  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;
  void ScrollIntoView(bool smooth, const UIBase* target,
                      const std::string& block,
                      const std::string& inline_value) override;

 protected:
  UIScroll(LynxContext* context, int sign, const std::string& tag);

  void HandleScrollStartEvent();

  void HandleScrollEvent(float delta_x, float delta_y);

  void HandleScrollStopEvent();

  void HandleContentSizeChangedEvent(float width, float height);
  void HandleScrollEdgeEvent();

  void SendCustomScrollEvent(const std::string name,
                             const std::pair<float, float> offset, float deltaX,
                             float deltaY);
  void UpdateContentSize(float width, float height) override;

 private:
  void OnScrollSticky(float x_offset, float y_offset);

  int UpdateBorderStatus(float x_offset, float y_offset);

  void InvokeScrollTo(
      int index, float offset, bool smooth,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void InvokeAutoScroll(
      float rate, bool start, bool auto_stop,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void InvokeGetScrollInfo(
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void ScrollToAsync();

 private:
  ArkUI_NodeHandle container_layout_{nullptr};
  std::shared_ptr<AutoScroller> auto_scroller_;
  int lower_threshold_{0};
  int upper_threshold_{0};
  int pending_scroll_index_{-1};
  float pending_scroll_left_{-1};
  float pending_scroll_top_{-1};
  int last_border_status_{kBorderStatusUpper};
  bool enable_scroll_upper_event_{false};
  bool enable_scroll_lower_event_{false};
  bool enable_scroll_event_{false};
  bool enable_scroll_start_event_{false};
  bool enable_scroll_stop_event_{false};
  bool enable_content_size_change_event_{false};
  bool enable_scroll_to_upper_edge_event_{false};
  bool enable_scroll_to_lower_edge_event_{false};
  bool enable_scroll_to_normal_state_event_{false};
  // edge_type
  static constexpr int kBorderStatusUpper = 1;
  static constexpr int kBorderStatusLower = 2;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
