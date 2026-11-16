// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture_handler/unified_gesture_handler.h"

namespace clay {

void UnifiedGestureHandler::OnGestureRecognized(
    unified_gesture::MemberId member_id) {}

void UnifiedGestureHandler::OnGestureStateChanged(
    unified_gesture::MemberId member_id, uint32_t gesture_id,
    unified_gesture::GestureState state) {}

void UnifiedGestureHandler::DispatchGestureEvent(
    const unified_gesture::GestureEvent &event) {
  // Touch-callback filtering must not suppress the raw recognition input.
  if (event.callback == unified_gesture::GestureCallbackType::kTouchesMove &&
      !has_touch_moved_) {
    return;
  }
  auto *view = FindMember(event.member_id);
  auto *delegate = page_view_->GetEventDelegate();
  if (!view || !delegate) {
    return;
  }
  const char *name = unified_gesture::GestureCallbackName(event.callback);
  Value::Map params;
  params.emplace("type",
                 Value(unified_gesture::GestureInputTypeName(event.source)));
  params.emplace("timestamp", Value(event.timestamp_epoch_ms));
  params.emplace("x", Value(event.local.x));
  params.emplace("y", Value(event.local.y));
  params.emplace("pageX", Value(event.page.x));
  params.emplace("pageY", Value(event.page.y));
  params.emplace("clientX", Value(event.client.x));
  params.emplace("clientY", Value(event.client.y));
  params.emplace("scrollX", Value(event.scroll.x));
  params.emplace("scrollY", Value(event.scroll.y));
  params.emplace("deltaX", Value(event.delta.x));
  params.emplace("deltaY", Value(event.delta.y));
  params.emplace("isAtStart", Value(event.scroll.is_at_start));
  params.emplace("isAtEnd", Value(event.scroll.is_at_end));
  delegate->OnUnifiedGestureHandlerEvent(name, view->Sign(), event.gesture_id,
                                         Value(std::move(params)));
}

}  // namespace clay
