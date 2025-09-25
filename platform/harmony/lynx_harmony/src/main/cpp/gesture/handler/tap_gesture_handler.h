// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_TAP_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_TAP_GESTURE_HANDLER_H_

#include <arkui/ui_input_event.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"

namespace lynx {
namespace base {
class TimedTaskManager;
}
namespace tasm {
class GestureDetector;
struct GestureCallback;

namespace harmony {
class LynxContext;
class GestureArenaMember;
class TouchEvent;

class TapGestureHandler : public BaseGestureHandler {
 public:
  TapGestureHandler(int sign, LynxContext* lynx_context,
                    std::shared_ptr<GestureDetector> gesture_detector,
                    std::weak_ptr<GestureArenaMember> gesture_arena_member);

  void HandleConfigMap(const lepus::Value& config);

  void OnHandle(
      const ArkUI_UIInputEvent* event,
      const std::shared_ptr<TouchEvent>& lynx_touch_event, float fling_delta_x,
      float fling_delta_y, bool handle_by_simultaneous,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) override;

  void Fail() override;
  void End() override;
  void Reset() override;

 protected:
  void OnBegin(float x, float y,
               const std::shared_ptr<TouchEvent>& event) override;
  void OnUpdate(
      float delta_x, float delta_y, const std::shared_ptr<TouchEvent>& event,
      const std::shared_ptr<GestureExtraBundle>& extra_bundle) override;
  void OnStart(float x, float y,
               const std::shared_ptr<TouchEvent>& event) override;
  void OnEnd(float x, float y,
             const std::shared_ptr<TouchEvent>& event) override;

 private:
  void StartTap();
  void EndTap();
  bool ShouldFail();

  float max_distance_;
  long max_duration_;
  float start_x_;
  float start_y_;
  float last_x_;
  float last_y_;
  bool is_invoked_end_;
  std::shared_ptr<TouchEvent> last_touch_event_;
  bool is_tap_active_;
  std::unique_ptr<base::TimedTaskManager> timer_task_manager_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_TAP_GESTURE_HANDLER_H_
