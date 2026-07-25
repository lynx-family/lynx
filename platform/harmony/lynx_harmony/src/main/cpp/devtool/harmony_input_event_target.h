// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_INPUT_EVENT_TARGET_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_INPUT_EVENT_TARGET_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "devtool/lynx_devtool/agent/input/input_event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxContext;

class HarmonyInputEventTarget final : public input::InputEventTarget {
 public:
  explicit HarmonyInputEventTarget(
      const std::shared_ptr<LynxContext>& lynx_context);
  ~HarmonyInputEventTarget() override;

  input::PointerCapabilities GetPointerCapabilities() const override;
  bool InjectPointerEvent(const input::PointerEvent& event) override;
  void WaitForInputProcessed(std::function<void(bool)> callback) override;

  void EmulateTouchFromMouseEvent(const std::string& event_type, int x, int y,
                                  const std::string& button, float delta_x,
                                  float delta_y, int modifiers,
                                  int click_count);
  void Shutdown();

 private:
  struct State {
    std::atomic_bool alive{true};
    bool touch_active = false;
    bool pending_injection_succeeded = true;
    int32_t active_pointer_id = 0;
    float active_touch_x = 0.f;
    float active_touch_y = 0.f;
  };

  static bool IsAvailable();
  static bool InjectPointerEventOnUIThread(LynxContext* context,
                                           const input::PointerEvent& event,
                                           State* state);
  static void EmulateTouchOnUIThread(LynxContext* context,
                                     const std::string& event_type, int x,
                                     int y, State* state);
  static bool InjectTouch(LynxContext* context, int32_t action,
                          int32_t pointer_id, float x, float y,
                          int64_t event_time_us);
  static bool ToEventPoint(LynxContext* context, float x, float y,
                           int32_t* display_x, int32_t* display_y,
                           int32_t* window_x, int32_t* window_y);
  static bool CancelActiveTouch(LynxContext* context, int64_t event_time_us,
                                State* state);
  static void ResetActiveTouch(State* state);

  std::weak_ptr<LynxContext> lynx_context_;
  std::shared_ptr<State> state_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_INPUT_EVENT_TARGET_H_
