// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_callback/scroll_event_callback_manager.h"

#include "clay/fml/logging.h"
#include "clay/ui/component/component_constants.h"

namespace clay {

ScrollEventCallbackManager::ScrollEventCallbackManager(BaseView* view,
                                                       int32_t callback_id)
    : view_(view), callback_id_(callback_id) {
  FML_DCHECK(view_);
  page_view_ = view_->page_view();
}

ScrollEventCallbackManager::~ScrollEventCallbackManager() = default;

void ScrollEventCallbackManager::NotifyScrolled(
    const FloatPoint& scrolled, const FloatPoint& offset,
    const BorderStatus& current_status, bool is_dragging) {
  HandleScrolled(scrolled, offset, current_status, is_dragging);
}

void ScrollEventCallbackManager::NotifyScrollStateChange(
    ScrollState old_state, ScrollState current_state, float velocity,
    bool is_dragging) const {
  if (ShouldSendEvent(kScrollStateChange)) {
    page_view_->SendEvent(
        callback_id_, event_attr::kEventScrollStateChange,
        {"currentState", "previousState", "velocity", "source"},
        static_cast<int>(current_state), static_cast<int>(old_state), velocity,
        is_dragging ? "dragging" : "code invocation");
  }
}

void ScrollEventCallbackManager::HandleScrolled(
    const FloatPoint& scrolled, const FloatPoint& offset,
    const BorderStatus& current_status, const bool is_dragging) {
  FML_DCHECK(view_ != nullptr);
  FloatSize zoomed_content = page_view_->ConvertTo<kPixelTypeLogical>(
      FloatSize(view_->ContentWidth(), view_->ContentHeight()));
  FloatPoint zoomed_offset = page_view_->ConvertTo<kPixelTypeLogical>(offset);
  FloatPoint zoomed_scroll = page_view_->ConvertTo<kPixelTypeLogical>(scrolled);
  auto now = fml::TimePoint::Now();
  if (ShouldSendEvent(kScrollEvent) &&
      now - last_scroll_event_ > scroll_event_throttle_) {
    last_scroll_event_ = now;
    SendScrollEvent(event_attr::kEventScroll, zoomed_scroll, zoomed_offset,
                    zoomed_content, is_dragging);
  }

  if (ShouldSendEvent(kScrollToUpper) && current_status.IsUpper() &&
      !last_status_.IsUpper()) {
    SendScrollEvent(event_attr::kEventScrollToUpper, zoomed_scroll,
                    zoomed_offset, zoomed_content, is_dragging);
  }

  if (ShouldSendEvent(kScrollToLower) && current_status.IsLower() &&
      !last_status_.IsLower()) {
    SendScrollEvent(event_attr::kEventScrollToLower, zoomed_scroll,
                    zoomed_offset, zoomed_content, is_dragging);
  }
  last_status_ = current_status;
}

void ScrollEventCallbackManager::NotifyScrollEnd(
    const FloatPoint& offset) const {
  if (ShouldSendEvent(kScrollEnd)) {
    FML_DCHECK(view_ != nullptr);

    FloatSize zoomed_content = page_view_->ConvertTo<kPixelTypeLogical>(
        FloatSize(view_->ContentWidth(), view_->ContentHeight()));
    FloatPoint zoomed_offset = page_view_->ConvertTo<kPixelTypeLogical>(offset);
    SendScrollEvent(event_attr::kEventScrollEnd, FloatPoint(0, 0),
                    zoomed_offset, zoomed_content);
  }
}

void ScrollEventCallbackManager::SendScrollEvent(
    const char* event_name, const FloatPoint& scrolled,
    const FloatPoint& offset, const FloatSize& content, const bool is_dragging,
    [[maybe_unused]] const EventSource event_source) const {
  page_view_->SendEvent(callback_id_, event_name,
                        {"scrollLeft", "scrollTop", "scrollHeight",
                         "scrollWidth", "deltaX", "deltaY", "isDragging"},
                        offset.x(), offset.y(), content.height(),
                        content.width(), scrolled.x(), scrolled.y(),
                        is_dragging);
}

}  // namespace clay
