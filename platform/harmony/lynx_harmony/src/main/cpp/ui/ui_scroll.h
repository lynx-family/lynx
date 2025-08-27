// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
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
static constexpr const char* const kInitialScrollToIndex =
    "initial-scroll-to-index";
static constexpr const char* const kInitialScrollOffset =
    "initial-scroll-offset";
// event
static constexpr const char* const kScrollEvent = "scroll";
static constexpr const char* const kScrollToUpperEvent = "scrolltoupper";
static constexpr const char* const kScrollToLowerEvent = "scrolltolower";
static constexpr const char* const kScrollToUpperEdgeEvent =
    "scrolltoupperedge";
static constexpr const char* const kScrollToLowerEdgeEvent =
    "scrolltoloweredge";
static constexpr const char* const kScrollToNormalStateEvent =
    "scrolltonormalstate";
static constexpr const char* const kScrollStartEvent = "scrollstart";
static constexpr const char* const kScrollEndEvent = "scrollend";
static constexpr const char* const kScrollToBounceEvent = "scrolltobounce";
static constexpr const char* const kContentSizeChangeEvent =
    "contentsizechanged";
// event_type
static constexpr ArkUI_NodeEventType kScrollNodeEventTypes[] = {
    NODE_SCROLL_EVENT_ON_SCROLL,      NODE_SCROLL_EVENT_ON_SCROLL_START,
    NODE_SCROLL_EVENT_ON_SCROLL_STOP, NODE_SCROLL_EVENT_ON_SCROLL_EDGE,
    NODE_SCROLL_EVENT_ON_WILL_SCROLL, NODE_SCROLL_EVENT_ON_DID_SCROLL};
// AlignmentOptions
static constexpr const char* const kNearest = "nearest";
static constexpr const char* const kCenter = "center";
static constexpr const char* const kEnd = "end";
static constexpr const char* const kStart = "start";
// const value
static constexpr int kInvalidIndex = -1;
static constexpr float kInvalidScrollOffset = -1.f;
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
  void HandleScrollBounceEvent();

  void SendCustomScrollEvent(const std::string name,
                             const std::pair<float, float> offset, float deltaX,
                             float deltaY);
  void UpdateContentSize(float width, float height) override;
  void FrameDidChanged() override;
  void EnableSticky() override;

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
  void ScrollToAsyncIfNeeded(const float scroll_offset);
  void ScrollToOffset(const float scroll_offset);
  void ResetEventFlag() {
    enable_scroll_to_upper_event_ = false;
    enable_scroll_to_lower_event_ = false;
    enable_scroll_event_ = false;
    enable_scroll_start_event_ = false;
    enable_scroll_end_event_ = false;
    enable_content_size_change_event_ = false;
    enable_scroll_to_upper_edge_event_ = false;
    enable_scroll_to_lower_edge_event_ = false;
    enable_scroll_to_normal_state_event_ = false;
    enable_scroll_to_bounce_event_ = false;
  }
  void ResetScrollTarget() {
    scroll_to_index_ = scroll::kInvalidIndex;
    scroll_left_ = scroll::kInvalidScrollOffset;
    scroll_top_ = scroll::kInvalidScrollOffset;
  }
  bool IsValidScrollToIndex(const int scroll_to_index) const {
    return scroll_to_index >= 0 &&
           scroll_to_index < static_cast<int>(children_.size());
  }
  bool IsValidScrollOffset(const float scroll_offset) const {
    return base::FloatsLargerOrEqual(scroll_offset, 0.f);
  }

 private:
  ArkUI_NodeHandle container_layout_{nullptr};
  std::shared_ptr<AutoScroller> auto_scroller_;
  int lower_threshold_{0};
  int upper_threshold_{0};
  bool should_consume_initial_scroll_target_{true};
  bool layout_changed_{false};
  bool enable_sticky_{false};
  int scroll_to_index_{scroll::kInvalidIndex};
  float scroll_left_{scroll::kInvalidScrollOffset};
  float scroll_top_{scroll::kInvalidScrollOffset};
  int initial_scroll_to_index_{scroll::kInvalidIndex};
  float initial_scroll_offset_{scroll::kInvalidScrollOffset};
  float pending_scroll_offset_{scroll::kInvalidScrollOffset};
  int last_border_status_{kBorderStatusUpper};
  bool enable_scroll_to_upper_event_{false};
  bool enable_scroll_to_lower_event_{false};
  bool enable_scroll_event_{false};
  bool enable_scroll_start_event_{false};
  bool enable_scroll_end_event_{false};
  bool enable_content_size_change_event_{false};
  bool enable_scroll_to_upper_edge_event_{false};
  bool enable_scroll_to_lower_edge_event_{false};
  bool enable_scroll_to_normal_state_event_{false};
  bool enable_scroll_to_bounce_event_{false};
  bool send_lower_bounces_event_{false};
  bool send_upper_bounces_event_{false};
  // edge_type
  static constexpr int kBorderStatusUpper = 1;
  static constexpr int kBorderStatusLower = 2;
  UIBase* end_bounce_view_{nullptr};
  UIBase* start_bounce_view_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_SCROLL_H_
