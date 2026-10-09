// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/gesture_handler_dispatcher.h"

#include <memory>
#include <utility>

#include "clay/ui/gesture_handler/unified_gesture_handler.h"

namespace clay {

GestureHandlerDispatcher::GestureHandlerDispatcher(PageView* page_view)
    : unified_gesture_handler_(
          std::make_shared<UnifiedGestureHandler>(page_view)) {}

GestureHandlerDispatcher::~GestureHandlerDispatcher() {
  auto handler = std::move(unified_gesture_handler_);
  handler->Reset();
}

int GestureHandlerDispatcher::ReplaceUnifiedMember(
    BaseView* view, const GestureMap& detectors) {
  auto handler = unified_gesture_handler_;
  return handler ? handler->ReplaceMember(view, detectors) : 0;
}

void GestureHandlerDispatcher::RemoveMember(int member_id) {
  if (auto handler = unified_gesture_handler_) {
    handler->RemoveMember(member_id);
  }
}

void GestureHandlerDispatcher::SetGestureState(int member_id,
                                               uint32_t gesture_id, int state) {
  if (auto handler = unified_gesture_handler_) {
    handler->SetGestureState(member_id, gesture_id, state);
  }
}

bool GestureHandlerDispatcher::CanControlGesture(int member_id,
                                                 uint32_t gesture_id) const {
  auto handler = unified_gesture_handler_;
  return handler && handler->ContainsGesture(member_id, gesture_id);
}

void GestureHandlerDispatcher::HandlePointerDown(const PointerEvent& event,
                                                 HitTestResult& hits) {
  gesture_recognized_target_set_.clear();
  if (auto handler = unified_gesture_handler_) {
    handler->HandlePointerEvent(event, hits);
  }
}

void GestureHandlerDispatcher::HandlePointerMove(const PointerEvent& event,
                                                 HitTestResult& hits) {
  if (auto handler = unified_gesture_handler_) {
    handler->HandlePointerEvent(event, hits);
  }
}

void GestureHandlerDispatcher::HandlePointerUp(const PointerEvent& event,
                                               HitTestResult& hits) {
  if (auto handler = unified_gesture_handler_) {
    handler->HandlePointerEvent(event, hits);
  }
}

void GestureHandlerDispatcher::HandlePointerCancel(const PointerEvent& event,
                                                   HitTestResult& hits) {
  if (auto handler = unified_gesture_handler_) {
    handler->HandlePointerEvent(event, hits);
  }
}

void GestureHandlerDispatcher::OnGestureRecognizedWithSign(int sign) {
  gesture_recognized_target_set_.insert(sign);
}

void GestureHandlerDispatcher::SetVelocity(FloatPoint velocity) {
  if (auto handler = unified_gesture_handler_) {
    handler->SetVelocity(velocity);
  }
}

}  // namespace clay
