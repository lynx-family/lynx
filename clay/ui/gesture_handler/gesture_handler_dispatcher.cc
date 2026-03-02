// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/gesture_handler_dispatcher.h"

#include <memory>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"

namespace clay {

GestureHandlerDispatcher::GestureHandlerDispatcher(PageView* page_view)
    : gesture_arena_manager_(
          std::make_unique<GestureArenaManager>(true, page_view)),
      velocity_tracker_(std::make_unique<VelocityTracker>()) {}

void GestureHandlerDispatcher::HandlePointerDown(
    const PointerEvent& pointer_event, HitTestResult& hit_test_result) {
  if (hit_test_result.empty()) return;
  gesture_recognized_target_set_.clear();
  gesture_arena_manager_->SetActiveUIToArenaAtDownEvent(hit_test_result);
  gesture_arena_manager_->DispatchTouchEventToArena(pointer_event);
}
void GestureHandlerDispatcher::HandlePointerMove(
    const PointerEvent& pointer_event, HitTestResult& hit_test_result) {
  if (hit_test_result.empty()) return;
  velocity_tracker_->AddPosition(pointer_event.position,
                                 pointer_event.timestamp);
  gesture_arena_manager_->DispatchTouchEventToArena(pointer_event);
}
void GestureHandlerDispatcher::HandlePointerUp(
    const PointerEvent& pointer_event, HitTestResult& hit_test_result) {
  if (hit_test_result.empty()) return;
  FloatSize v = velocity_tracker_->GetVelocityEstimate().pixels_per_second;
  gesture_arena_manager_->SetVelocity(v.width(), v.height());
  gesture_arena_manager_->DispatchTouchEventToArena(pointer_event);
}
void GestureHandlerDispatcher::HandlePointerCancel(
    const PointerEvent& pointer_event, HitTestResult& hit_test_result) {
  if (hit_test_result.empty()) return;
  gesture_arena_manager_->DispatchTouchEventToArena(pointer_event);
}

void GestureHandlerDispatcher::OnGestureRecognizedWithSign(int sign) {
  gesture_recognized_target_set_.insert(sign);
}

void GestureHandlerDispatcher::SetVelocity(FloatPoint velocity) {
  gesture_arena_manager_->SetVelocity(velocity.x(), velocity.y());
}

void GestureHandlerDispatcher::DispatchSingleTouchEvent(
    const std::string& name, const PointerEvent* pointer_event) {}

}  // namespace clay
