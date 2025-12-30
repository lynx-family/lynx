// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_DEFAULT_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_DEFAULT_GESTURE_HANDLER_H_

#include <arkui/ui_input_event.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"

namespace lynx {
namespace tasm {
class GestureDetector;
struct GestureCallback;

namespace harmony {
class LynxContext;
class GestureArenaMember;
class TouchEvent;

class DefaultGestureHandler : public BaseGestureHandler {
 public:
  DefaultGestureHandler(int sign, LynxContext* lynx_context,
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
  lepus::Value GetEventParamsInActive(
      const std::shared_ptr<TouchEvent>& touch_event, float delta_x,
      float delta_y);
  bool ShouldFail(float delta_x, float delta_y,
                  const std::shared_ptr<GestureExtraBundle>& extra_bundle);

  float last_x_;
  float last_y_;
  bool is_invoked_begin_;
  bool is_invoked_start_;
  bool is_invoked_end_;
  int tap_slop_;
  std::shared_ptr<TouchEvent> last_touch_event_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_DEFAULT_GESTURE_HANDLER_H_
