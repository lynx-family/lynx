// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_

#include <memory>
#include <unordered_set>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture_handler/gesture_detector.h"

namespace clay {

class PageView;
class BaseView;
class UnifiedGestureHandler;

class GestureHandlerDispatcher {
 public:
  explicit GestureHandlerDispatcher(PageView* page_view);
  ~GestureHandlerDispatcher();

  int ReplaceUnifiedMember(BaseView* view, const GestureMap& detectors);
  void RemoveMember(int member_id);
  void SetGestureState(int member_id, uint32_t gesture_id, int state);
  bool CanControlGesture(int member_id, uint32_t gesture_id) const;

  void HandlePointerDown(const PointerEvent& pointer_event,
                         HitTestResult& hit_test_result);
  void HandlePointerMove(const PointerEvent& pointer_event,
                         HitTestResult& hit_test_result);
  void HandlePointerUp(const PointerEvent& pointer_event,
                       HitTestResult& hit_test_result);
  void HandlePointerCancel(const PointerEvent& pointer_event,
                           HitTestResult& hit_test_result);

  void OnGestureRecognizedWithSign(int sign);
  void SetVelocity(FloatPoint velocity);
  const std::unordered_set<int> GetGestureRecognizedTargetSet() const {
    return gesture_recognized_target_set_;
  }

 private:
  std::shared_ptr<UnifiedGestureHandler> unified_gesture_handler_;
  std::unordered_set<int> gesture_recognized_target_set_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_HANDLER_DISPATCHER_H_
