// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_GESTURE_HANDLER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_GESTURE_HANDLER_H_

#include <memory>

#include "clay/public/value.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

class GestureDetector;
class GestureArenaMember;

class FlingGestureHandler : public BaseGestureHandler {
 public:
  FlingGestureHandler(int sign, PageView* page_view,
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

  Value GenerateAdditionalEventParams(float delta_x, float delta_y);

 private:
  bool is_invoked_begin_;
  bool is_invoked_start_;
  bool is_invoked_end_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_GESTURE_HANDLER_H_
