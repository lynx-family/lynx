// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture/velocity_tracker.h"
#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"

namespace clay {

class PageView;

class GestureHandlerDispatcher {
 public:
  explicit GestureHandlerDispatcher(PageView* page_view);

  void SetEnableMultiTouch(bool enable_multi_touch) {
    enable_multi_touch_ = enable_multi_touch;
  }

  void HandlePointerDown(const PointerEvent& pointer_event,
                         HitTestResult& hit_test_result);
  void HandlePointerMove(const PointerEvent& pointer_event,
                         HitTestResult& hit_test_result);
  void HandlePointerUp(const PointerEvent& pointer_event,
                       HitTestResult& hit_test_result);
  void HandlePointerCancel(const PointerEvent& pointer_event,
                           HitTestResult& hit_test_result);

  GestureArenaManager* gesture_arena_manager() {
    return gesture_arena_manager_.get();
  }

  void OnGestureRecognizedWithSign(int sign);
  void SetVelocity(FloatPoint velocity);
  const std::unordered_set<int> GetGestureRecognizedTargetSet() const {
    return gesture_recognized_target_set_;
  }

 private:
  void DispatchSingleTouchEvent(const std::string& name,
                                const PointerEvent* pointer_event);

  std::unique_ptr<GestureArenaManager> gesture_arena_manager_;
  bool enable_multi_touch_{false};
  std::unordered_set<int> gesture_recognized_target_set_;
  std::unique_ptr<VelocityTracker> velocity_tracker_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_
