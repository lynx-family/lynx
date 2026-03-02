// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_LONGPRESS_GESTURE_HANDLER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_LONGPRESS_GESTURE_HANDLER_H_

#include <memory>

#include "base/include/fml/time/timer.h"
#include "clay/public/value.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

class GestureDetector;
class GestureArenaMember;

class LongPressGestureHandler : public BaseGestureHandler {
 public:
  LongPressGestureHandler(
      int sign, PageView* page_view,
      std::shared_ptr<GestureDetector> gesture_detector,
      fml::WeakPtr<GestureArenaMember> gesture_arena_member);

  void HandleConfigMap(const Value& config);

  void OnHandle(
      const PointerEvent* pointer_event, float fling_delta_x,
      float fling_delta_y, bool handle_by_simultaneous,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) override;

  void Fail() override;
  void End() override;
  void Reset() override;

 protected:
  void OnBegin(float x, float y, const PointerEvent* pointer_event) override;
  void OnUpdate(
      float delta_x, float delta_y, const PointerEvent* pointer_event,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) override;
  void OnStart(float x, float y, const PointerEvent* pointer_event) override;
  void OnEnd(float x, float y, const PointerEvent* pointer_event) override;

 private:
  void StartLongPress();
  void EndLongPress();
  bool ShouldFail();

  float max_distance_;
  long min_duration_;
  FloatPoint start_point_;
  FloatPoint last_point_;
  bool is_invoked_end_;

  const PointerEvent* last_pointer_event_{nullptr};
  fml::OneshotTimer timer_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_LONGPRESS_GESTURE_HANDLER_H_
