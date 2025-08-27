// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h"

#include <algorithm>
#include <limits>

#include "base/include/float_comparison.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/utils/value_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_bounce.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

UIScroll::UIScroll(LynxContext* context, int sign, const std::string& tag)
    : BaseScrollContainer(context, sign, tag) {
  container_layout_ = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);
  NodeManager::Instance().InsertNode(node_, container_layout_, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      node_, NODE_ALIGNMENT, static_cast<int32_t>(ARKUI_ALIGNMENT_TOP_START));
  NodeManager::Instance().AddNodeEventReceiver(container_layout_,
                                               UIBase::EventReceiver);
  NodeManager::Instance().AddNodeCustomEventReceiver(
      container_layout_, UIBase::CustomEventReceiver);

  for (auto eventType : scroll::kScrollNodeEventTypes) {
    NodeManager::Instance().RegisterNodeEvent(Node(), eventType, eventType,
                                              this);
  }
  NodeManager::Instance().RegisterNodeCustomEvent(
      container_layout_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE,
      ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE, this);
  auto_scroller_ = std::make_shared<AutoScroller>(this);
}

UIScroll::~UIScroll() {
  NodeManager::Instance().UnregisterNodeCustomEvent(
      container_layout_, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
  for (auto eventType : scroll::kScrollNodeEventTypes) {
    NodeManager::Instance().UnregisterNodeEvent(node_, eventType);
  }

  NodeManager::Instance().RemoveNodeEventReceiver(container_layout_,
                                                  UIBase::EventReceiver);
  NodeManager::Instance().RemoveNodeCustomEventReceiver(
      container_layout_, UIBase::CustomEventReceiver);
  NodeManager::Instance().DisposeNode(container_layout_);

  end_bounce_view_ = nullptr;
  start_bounce_view_ = nullptr;
}

void UIScroll::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (method == "scrollTo") {
    if (!(args.IsTable())) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not table!"));
    }
    // Compatible with historical logic without index parameter
    int index{-1};
    float offset{0};
    bool smooth{false};
    const auto table = args.Table();
    for (auto& [k, v] : *table) {
      if (k.IsEqual("index")) {
        if (v.IsNumber()) {
          index = static_cast<int>(v.Number());
          if (index < 0 || index >= children_.size()) {
            callback(LynxGetUIResult::PARAM_INVALID,
                     lepus::Value(base::FormatString(
                         "on the scrollTo() method, the index  %d is out of "
                         "range [ 0, %d ]",
                         index, children_.size())));
            return;
          }
        } else {
          index = -1;
        }
      } else if (k.IsEqual("offset") && v.IsNumber()) {
        offset = v.Number();
      } else if (k.IsEqual("smooth") && v.IsBool()) {
        smooth = v.Bool();
      }
    }
    InvokeScrollTo(index, offset, smooth, std::move(callback));
  } else if (method == "autoScroll") {
    float rate{0.f};
    bool start{false};
    bool auto_stop{true};

    if (!args.IsTable()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not table!"));
    }
    const auto& table = *args.Table();
    for (const auto& [k, v] : table) {
      if (k.IsEqual("rate")) {
        if (v.IsNumber()) {
          rate = v.Number();
        } else if (v.IsString()) {
          float screen_size[2] = {0};
          context_->ScreenSize(screen_size);
          rate = LynxUnitUtils::ToVPFromUnitValue(v.StdString(), screen_size[0],
                                                  context_->DevicePixelRatio());
        }
      } else if (k.IsEqual("start") && v.IsBool()) {
        start = v.Bool();
      } else if (k.IsEqual("autoStop") && v.IsBool()) {
        auto_stop = v.Bool();
      }
    }
    InvokeAutoScroll(rate, start, auto_stop, std::move(callback));
  } else if (method == "getScrollInfo") {
    InvokeGetScrollInfo(std::move(callback));
  } else if (method == "scrollBy") {
    if (!args.IsTable()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("params is not object!"));
      return;
    }
    float offset{0};
    auto table = args.Table();
    for (const auto& [k, v] : *table) {
      if (k.IsEqual("offset") && v.IsNumber()) {
        offset = v.Number();
      }
    }
    std::vector<float> result = BaseScrollContainer::ScrollBy(offset, offset);
    auto dictionary = lepus::Dictionary::Create();
    if (result.size() >= 4) {
      dictionary->SetValue("consumedX", result[0]);
      dictionary->SetValue("consumedY", result[1]);
      dictionary->SetValue("unconsumedX", result[2]);
      dictionary->SetValue("unconsumedY", result[3]);
    }
    callback(LynxGetUIResult::SUCCESS, lepus_value(std::move(dictionary)));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIScroll::ScrollIntoView(bool smooth, const UIBase* target,
                              const std::string& block,
                              const std::string& inline_value) {
  if (!target) {
    return;
  }
  float scroll_distance = 0;
  if (IsHorizontal()) {
    if (scroll::kNearest == inline_value) {
      return;
    }
    if (scroll::kCenter == inline_value) {
      scroll_distance -= (width_ - target->width_) / 2;
    } else if (scroll::kEnd == inline_value) {
      scroll_distance -= (width_ - target->width_);
    }
    while (target && target != this) {
      scroll_distance += target->left_;
      target = target->Parent();
    }
    scroll_distance =
        std::max(0.f, std::min(scroll_distance, content_width_ - width_));
    ScrollTo(scroll_distance, 0, smooth);
  } else {
    if (scroll::kNearest == block) {
      return;
    }
    if (scroll::kCenter == block) {
      scroll_distance -= (height_ - target->height_) / 2;
    } else if (scroll::kEnd == block) {
      scroll_distance -= (height_ - target->height_);
    }
    while (target && target != this) {
      scroll_distance += target->top_;
      target = target->Parent();
    }
    scroll_distance =
        std::max(0.f, std::min(scroll_distance, content_height_ - height_));
    ScrollTo(0, scroll_distance, smooth);
  }
}

void UIScroll::OnMeasure(ArkUI_LayoutConstraint* layout_constraint) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_SCROLL_ON_MEASURE);
  float content_width = width_;
  float content_height = height_;
  // set the layout_constraint for the container view.
  ArkUI_LayoutConstraint* constraint = OH_ArkUI_LayoutConstraint_Create();
  OH_ArkUI_LayoutConstraint_SetMinHeight(constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxHeight(constraint,
                                         std::numeric_limits<int32_t>::max());
  OH_ArkUI_LayoutConstraint_SetMinWidth(constraint, 0);
  OH_ArkUI_LayoutConstraint_SetMaxWidth(constraint,
                                        std::numeric_limits<int32_t>::max());
  for (const auto child : children_) {
    if (child) {
      NodeManager::Instance().MeasureNode(child->DrawNode(), constraint);
      if (IsHorizontal()) {
        content_width =
            std::max(content_width, child->width_ + child->left_ +
                                        padding_right_ + child->margin_right_);
      } else {
        content_height = std::max(content_height, child->height_ + child->top_ +
                                                      padding_bottom_ +
                                                      child->margin_bottom_);
      }
    }
  }
  if (start_bounce_view_ != nullptr) {
    NodeManager::Instance().MeasureNode(start_bounce_view_->DrawNode(),
                                        constraint);
    NodeManager::Instance().SetAttributeWithNumberValue(
        start_bounce_view_->DrawNode(), NODE_POSITION,
        IsHorizontal() ? -start_bounce_view_->width_ : 0,
        IsHorizontal() ? 0 : -start_bounce_view_->height_);
  }

  if (end_bounce_view_ != nullptr) {
    NodeManager::Instance().MeasureNode(end_bounce_view_->DrawNode(),
                                        constraint);
    NodeManager::Instance().SetAttributeWithNumberValue(
        end_bounce_view_->DrawNode(), NODE_POSITION,
        IsHorizontal() ? content_width : 0,
        IsHorizontal() ? 0 : content_height);
  }
  OH_ArkUI_LayoutConstraint_Dispose(constraint);
  if (!base::FloatsEqual(content_width, content_width_) ||
      !base::FloatsEqual(content_height, content_height_)) {
    UpdateContentSize(content_width, content_height);
  }
  layout_changed_ = false;
  if (IsValidScrollOffset(pending_scroll_offset_)) {
    ScrollToOffset(pending_scroll_offset_);
    pending_scroll_offset_ = scroll::kInvalidScrollOffset;
  }
}

void UIScroll::OnNodeReady() {
  BaseScrollContainer::OnNodeReady();
  float scroll_offset = scroll::kInvalidScrollOffset;
  if (IsValidScrollToIndex(scroll_to_index_)) {
    UIBase* child = children_[scroll_to_index_];
    scroll_offset = is_horizontal_ ? child->left_ : child->top_;
  } else if (IsValidScrollOffset(is_horizontal_ ? scroll_left_ : scroll_top_)) {
    scroll_offset = is_horizontal_ ? scroll_left_ : scroll_top_;
  }
  // Reset scroll_to_index_ / scroll_left_ / scroll_top_.
  ResetScrollTarget();
  // Consume valid initial-scroll-to-index or initial-scroll-offset.
  if (should_consume_initial_scroll_target_) {
    should_consume_initial_scroll_target_ = false;
    if (IsValidScrollToIndex(initial_scroll_to_index_)) {
      UIBase* child = children_[initial_scroll_to_index_];
      scroll_offset = is_horizontal_ ? child->left_ : child->top_;
    } else if (IsValidScrollOffset(initial_scroll_offset_)) {
      scroll_offset = initial_scroll_offset_;
    }
  }
  if (IsValidScrollOffset(scroll_offset)) {
    ScrollToOffset(scroll_offset);
  }
}

void UIScroll::ScrollToOffset(const float scroll_offset) {
  if (layout_changed_) {
    pending_scroll_offset_ = scroll_offset;
    return;
  }
  ScrollToAsyncIfNeeded(scroll_offset);
}

void UIScroll::ScrollToAsyncIfNeeded(const float scroll_offset) {
  const auto& monitor = context_->VSyncMonitor();
  if (monitor) {
    monitor->ScheduleVSyncSecondaryCallback(
        reinterpret_cast<intptr_t>(this),
        [weak_this = weak_from_this(), scroll_offset](int64_t, int64_t) {
          auto share_this = weak_this.lock();
          if (share_this) {
            UIScroll* ui_scroll = static_cast<UIScroll*>(share_this.get());
            ui_scroll->ScrollTo(ui_scroll->is_horizontal_ ? scroll_offset : 0.f,
                                ui_scroll->is_horizontal_ ? 0.f : scroll_offset,
                                false);
          }
        });
  }
}

void UIScroll::OnPropUpdate(const std::string& name,
                            const lepus::Value& value) {
  if (name == scroll::kScrollX && value.IsBool()) {
    // TODO: @deprecated scroll-x
    SetHorizontal(value.Bool());
  } else if (name == scroll::kScrollY && value.IsBool()) {
    // TODO: @deprecated scroll-y
    SetHorizontal(!value.Bool());
  } else if (name == scroll::kEnableScroll && value.IsBool()) {
    SetEnableScrollInteraction(value.Bool());
  } else if (name == scroll::kEnableNestedScroll && value.IsBool()) {
    SetNestedScroll(value.Bool());
  } else if (name == scroll::kEnableScrollBar && value.IsBool()) {
    SetScrollbar(value.Bool());
  } else if (name == scroll::kBounces && value.IsBool()) {
    SetBounces(value.Bool());
  } else if (name == scroll::kLowerThreshold && value.IsNumber()) {
    lower_threshold_ = static_cast<int>(value.Number());
  } else if (name == scroll::kUpperThreshold && value.IsNumber()) {
    upper_threshold_ = static_cast<int>(value.Number());
  } else if (name == scroll::kScrollToIndex && value.IsNumber()) {
    scroll_to_index_ = static_cast<int>(value.Number());
  } else if (name == scroll::kScrollLeft && value.IsNumber()) {
    scroll_left_ = value.Number();
  } else if (name == scroll::kScrollTop && value.IsNumber()) {
    scroll_top_ = value.Number();
  } else if (name == scroll::kInitialScrollToIndex && value.IsNumber()) {
    initial_scroll_to_index_ = static_cast<int>(value.Number());
  } else if (name == scroll::kInitialScrollOffset && value.IsNumber()) {
    initial_scroll_offset_ = value.Number();
  } else {
    BaseScrollContainer::OnPropUpdate(name, value);
  }
}

void UIScroll::SetEvents(const std::vector<lepus::Value>& events) {
  UIBase::SetEvents(events);
  ResetEventFlag();
  for (const auto& event : events_) {
    if (event == scroll::kScrollEvent) {
      enable_scroll_event_ = true;
    } else if (event == scroll::kContentSizeChangeEvent) {
      enable_content_size_change_event_ = true;
    } else if (event == scroll::kScrollStartEvent) {
      enable_scroll_start_event_ = true;
    } else if (event == scroll::kScrollEndEvent) {
      enable_scroll_end_event_ = true;
    } else if (event == scroll::kScrollToLowerEvent) {
      enable_scroll_to_lower_event_ = true;
    } else if (event == scroll::kScrollToUpperEvent) {
      enable_scroll_to_upper_event_ = true;
    } else if (event == scroll::kScrollToLowerEdgeEvent) {
      enable_scroll_to_lower_edge_event_ = true;
    } else if (event == scroll::kScrollToUpperEdgeEvent) {
      enable_scroll_to_upper_edge_event_ = true;
    } else if (event == scroll::kScrollToNormalStateEvent) {
      enable_scroll_to_normal_state_event_ = true;
    } else if (event == scroll::kScrollToBounceEvent) {
      enable_scroll_to_bounce_event_ = true;
    }
  }
}

void UIScroll::AddChild(lynx::tasm::harmony::UIBase* child, int index) {
  bool is_bounce_view =
      child->Tag() == "bounce-view" || child->Tag() == "x-bounce-view";
  if (index == -1) {
    children_.emplace_back(child);
  } else if (!is_bounce_view) {
    children_.insert(children_.begin() + index, child);
  }
  if (is_bounce_view) {
    if (static_cast<UIBounce*>(child)->is_lower_) {
      start_bounce_view_ = child;
    } else {
      end_bounce_view_ = child;
    }
    NodeManager::Instance().InsertNode(container_layout_, child->DrawNode(),
                                       index);
    layout_changed_ = true;
    return;
  }
  child->SetParent(this);
  NodeManager::Instance().InsertNode(container_layout_, child->DrawNode(),
                                     index);
  layout_changed_ = true;
}

void UIScroll::RemoveChild(lynx::tasm::harmony::UIBase* child) {
  child->SetParent(nullptr);
  NodeManager::Instance().RemoveNode(container_layout_, child->DrawNode());
  layout_changed_ = true;
  if (child == end_bounce_view_) {
    end_bounce_view_ = nullptr;
    return;
  } else if (child == start_bounce_view_) {
    start_bounce_view_ = nullptr;
    return;
  }
  children_.erase(std::remove(children_.begin(), children_.end(), child),
                  children_.end());
}

void UIScroll::FrameDidChanged() {
  UIBase::FrameDidChanged();
  layout_changed_ = true;
}

void UIScroll::UpdateContentSize(float width, float height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_SCROLL_UPDATE_CONTENT_SIZE);
  BaseScrollContainer::UpdateContentSize(width, height);
  NodeManager::Instance().SetMeasuredSize(container_layout_,
                                          context_->ScaledDensity() * width,
                                          context_->ScaledDensity() * height);
  HandleContentSizeChangedEvent(width, height);
  HandleScrollEdgeEvent();
}

void UIScroll::OnNodeEvent(ArkUI_NodeEvent* event) {
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);

  if (type == NODE_SCROLL_EVENT_ON_SCROLL_START) {
    HandleScrollStartEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL) {
    GestureRecognized();
    auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    HandleScrollEvent(component_event->data[0].f32,
                      component_event->data[1].f32);
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL_STOP) {
    HandleScrollStopEvent();
    HandleScrollBounceEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_SCROLL_EDGE) {
    HandleScrollEdgeEvent();
  } else if (type == NODE_SCROLL_EVENT_ON_WILL_SCROLL) {
    if (IsEnableNewGesture() && !consume_gesture_) {
      auto* component_event = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
      component_event->data[0].f32 = 0.f;
      component_event->data[1].f32 = 0.f;
    }
  } else if (type == NODE_SCROLL_EVENT_ON_DID_SCROLL) {
    // Note: In the case of that the content size has changed which leads to the
    // scroll view set a new content offset, only
    // NODE_SCROLL_EVENT_ON_DID_SCROLL event is triggered by system, so we need
    // to handle translation for sticky child nodes here. At the same time,
    // considering if the NODE_SCROLL_EVENT_ON_SCROLL is triggered, the
    // NODE_SCROLL_EVENT_ON_DID_SCROLL will definitely be triggered as well, so
    // we uniformly call OnScrollSticky here.
    const auto& scroll_offset = GetScrollOffset();
    OnScrollSticky(scroll_offset.first, scroll_offset.second);
  } else {
    UIBase::OnNodeEvent(event);
  }
}

void UIScroll::HandleScrollBounceEvent() {
  if (!enable_scroll_to_bounce_event_) {
    return;
  }
  if (send_lower_bounces_event_) {
    send_lower_bounces_event_ = false;
    auto param = lepus::Dictionary::Create();
    param->SetValue("direction", IsHorizontal() ? "left" : "bottom");
    CustomEvent event{Sign(), scroll::kScrollToBounceEvent, "detail",
                      lepus_value(param)};
    context_->SendEvent(event);
  }
  if (send_upper_bounces_event_) {
    send_upper_bounces_event_ = false;
    auto param = lepus::Dictionary::Create();
    param->SetValue("direction", IsHorizontal() ? "right" : "top");
    CustomEvent event{Sign(), scroll::kScrollToBounceEvent, "detail",
                      lepus_value(param)};
    context_->SendEvent(event);
    SendCustomScrollEvent(scroll::kScrollToBounceEvent, GetScrollOffset(), 0,
                          0);
  }
}

void UIScroll::HandleScrollEdgeEvent() {
  float scroll_range = is_horizontal_ ? content_width_ : content_height_;
  float scroll_offset = GetScrollDistance();
  float scroll_view_size = is_horizontal_ ? width_ : height_;
  bool is_lower_edge = false;
  bool is_upper_edge = false;
  if (scroll_range < scroll_view_size) {
    is_lower_edge = true;
    is_upper_edge = true;
  } else {
    if (scroll_range <= scroll_offset + scroll_view_size) {
      is_lower_edge = true;
    }
    if (scroll_offset <= 0) {
      is_upper_edge = true;
    }
  }
  if (enable_scroll_to_upper_edge_event_ && is_upper_edge) {
    SendCustomScrollEvent(scroll::kScrollToUpperEdgeEvent, GetScrollOffset(), 0,
                          0);
  }
  if (enable_scroll_to_lower_edge_event_ && is_lower_edge) {
    SendCustomScrollEvent(scroll::kScrollToLowerEdgeEvent, GetScrollOffset(), 0,
                          0);
  }
  if (enable_scroll_to_normal_state_event_ && !is_lower_edge &&
      !is_upper_edge) {
    SendCustomScrollEvent(scroll::kScrollToNormalStateEvent, GetScrollOffset(),
                          0, 0);
  }
}

void UIScroll::HandleScrollStartEvent() {
  if (enable_scroll_start_event_) {
    auto offset = GetScrollOffset();
    SendCustomScrollEvent(scroll::kScrollStartEvent, offset, 0, 0);
  }
  // TODO(zhangkaijie.9): "tag" may need be replaced by prop
  // 'scroll-monitor-tag'
  context_->StartFluencyTrace(Sign(), harmony::kFluencyScrollEvent, "tag");
}

void UIScroll::HandleScrollEvent(float delta_x, float delta_y) {
  auto offset = GetScrollOffset();
  // onScrollEvent
  if (end_bounce_view_ != nullptr) {
    if (IsHorizontal()) {
      if (offset.first >= content_width_ + end_bounce_view_->width_ - width_) {
        send_upper_bounces_event_ = true;
      }
    } else {
      if (offset.second >=
          content_height_ + end_bounce_view_->height_ - height_) {
        send_upper_bounces_event_ = true;
      }
    }
  }
  if (start_bounce_view_ != nullptr) {
    if (IsHorizontal()) {
      if (offset.first <= -start_bounce_view_->width_) {
        send_lower_bounces_event_ = true;
      }
    } else {
      if (offset.second <= -start_bounce_view_->height_) {
        send_lower_bounces_event_ = true;
      }
    }
  }
  if (enable_scroll_event_) {
    SendCustomScrollEvent(scroll::kScrollEvent, offset, delta_x, delta_y);
  }
  // onScrollLowerEvent、onScrollUpperEvent
  if (enable_scroll_to_upper_event_ || enable_scroll_to_lower_event_) {
    auto status = UpdateBorderStatus(offset.first, offset.second);
    if (enable_scroll_to_upper_event_ && status == kBorderStatusUpper &&
        last_border_status_ != kBorderStatusUpper) {
      this->SendCustomScrollEvent(scroll::kScrollToUpperEvent, offset, 0, 0);
    } else if (enable_scroll_to_lower_event_ && status == kBorderStatusLower &&
               last_border_status_ != kBorderStatusLower) {
      SendCustomScrollEvent(scroll::kScrollToLowerEvent, offset, 0, 0);
    }
    last_border_status_ = status;
  }

  // NODE_SCROLL_EVENT_ON_SCROLL will be sent even when the scroll-view is at
  // the edge and can not be scrolled. So the scrolltoedge event can not be sent
  // here. And the normal state event has to judge whether this scroll event is
  // a legal one.
  if (enable_scroll_to_normal_state_event_) {
    float list_size = is_horizontal_ ? width_ : height_;
    float scroll_range = is_horizontal_ ? content_width_ : content_height_;
    if (GetScrollDistance() > 0 ||
        (list_size < scroll_range &&
         GetScrollDistance() < scroll_range - list_size)) {
      SendCustomScrollEvent(scroll::kScrollToNormalStateEvent, offset, 0, 0);
    }
  }
  context_->NotifyUIScroll();
}

void UIScroll::HandleScrollStopEvent() {
  if (enable_scroll_end_event_) {
    auto offset = this->GetScrollOffset();
    SendCustomScrollEvent(scroll::kScrollEndEvent, offset, 0, 0);
  }
  context_->StopFluencyTrace(Sign());
}

void UIScroll::HandleContentSizeChangedEvent(float width, float height) {
  if (!enable_content_size_change_event_) {
    return;
  }
  auto param = lepus::Dictionary::Create();
  param->SetValue("scrollWidth", width);
  param->SetValue("scrollHeight", height);
  CustomEvent event{Sign(), scroll::kContentSizeChangeEvent, "detail",
                    lepus_value(param)};
  context_->SendEvent(event);
}

void UIScroll::SendCustomScrollEvent(const std::string name,
                                     const std::pair<float, float> offset,
                                     float delta_x, float delta_y) {
  auto param = lepus::Dictionary::Create();
  param->SetValue("scrollLeft", offset.first);
  param->SetValue("scrollTop", offset.second);
  param->SetValue("scrollWidth", content_width_);
  param->SetValue("scrollHeight", content_height_);
  param->SetValue("deltaX", delta_x);
  param->SetValue("deltaY", delta_y);
  CustomEvent event{Sign(), name, "detail", lepus_value(param)};
  context_->SendEvent(event);
}

void UIScroll::EnableSticky() {
  // Note: EnableSticky() is invoked from UIBase::UpdateSticky() which means
  // that the child node's sticky info has updated, so we need to invoke
  // OnScrollSticky() here to handle only sticky info changing' case.
  enable_sticky_ = true;
  const auto& offset = GetScrollOffset();
  OnScrollSticky(offset.first, offset.second);
}

void UIScroll::OnScrollSticky(float x_offset, float y_offset) {
  if (!enable_sticky_) {
    return;
  }
  for (const auto child : children_) {
    if (child) {
      child->CheckStickyOnParentScroll(x_offset, y_offset);
      if (!child->sticky_value_.empty()) {
        NodeManager::Instance().SetAttributeWithNumberValue(
            child->DrawNode(), NODE_TRANSLATE, 0, child->sticky_value_[5], 0);
        NodeManager::Instance().SetAttributeWithNumberValue(child->DrawNode(),
                                                            NODE_Z_INDEX, 1);
      }
    }
  }
}

void UIScroll::InvokeScrollTo(
    int index, float offset, bool smooth,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (children_.empty()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus::Value("Invoke scrollTo() failed due to empty children."));
    return;
  }

  bool is_horizontal = IsHorizontal();
  float can_scroll_distance = is_horizontal ? content_width_ : content_height_;
  if (index >= 0 && index < children_.size()) {
    UIBase* targetView = children_[index];
    // calculate the offset and distance
    if (targetView) {
      if (is_horizontal) {
        offset += targetView->left_;
      } else {
        offset += targetView->top_;
      }
    }
  }

  if (offset < 0 || offset > can_scroll_distance) {
    callback(LynxGetUIResult::PARAM_INVALID, lepus_value(""));
  } else {
    ScrollTo((is_horizontal ? offset : 0), (is_horizontal ? 0 : offset),
             smooth);
    callback(
        LynxGetUIResult::SUCCESS,
        lepus_value(base::FormatString("Target scroll position %d is beyond "
                                       "threshold.  [0,  %d ]",
                                       offset, can_scroll_distance)));
  }
}

void UIScroll::InvokeAutoScroll(
    float rate, bool start, bool auto_stop,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto_scroller_) {
    auto_scroller_->AutoScroll(rate, start, auto_stop, std::move(callback));
  }
}
void UIScroll::InvokeGetScrollInfo(
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  auto offset = GetScrollOffset();
  auto scroll_range = IsHorizontal() ? content_width_ : content_height_;
  auto info = lepus::Dictionary::Create();
  info->SetValue("scrollX", offset.first);
  info->SetValue("scrollY", offset.second);
  info->SetValue("scrollRange", scroll_range - GetViewPortSize());
  callback(LynxGetUIResult::SUCCESS, lepus_value(info));
}

int UIScroll::UpdateBorderStatus(float x_offset, float y_offset) {
  int status = 0;
  if (IsHorizontal()) {  // scroll-x
    // should be converted to int_64 for comparison
    bool is_upper =
        static_cast<int64_t>(context_->ScaledDensity() * upper_threshold_) >=
        static_cast<int64_t>(context_->ScaledDensity() * x_offset);
    bool is_lower =
        static_cast<int64_t>(context_->ScaledDensity() * x_offset) >=
        static_cast<int64_t>(context_->ScaledDensity() *
                             (content_width_ - width_ - lower_threshold_));

    if (is_upper) {
      status = kBorderStatusUpper;
    } else if (is_lower) {
      status = kBorderStatusLower;
    }
  } else {  // scroll-y
            // should be converted to int_64 for comparison
    bool is_upper =
        static_cast<int64_t>(context_->ScaledDensity() * upper_threshold_) >=
        static_cast<int64_t>(context_->ScaledDensity() * y_offset);
    bool is_lower =
        static_cast<int64_t>(context_->ScaledDensity() * y_offset) >=
        static_cast<int64_t>(context_->ScaledDensity() *
                             (content_height_ - height_ - lower_threshold_));

    if (is_upper) {
      status = kBorderStatusUpper;
    } else if (is_lower) {
      status = kBorderStatusLower;
    }
  }
  return status;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
