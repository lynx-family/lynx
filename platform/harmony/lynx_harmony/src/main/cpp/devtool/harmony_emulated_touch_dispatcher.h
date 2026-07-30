// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_EMULATED_TOUCH_DISPATCHER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_EMULATED_TOUCH_DISPATCHER_H_

#include <cstdint>
#include <memory>
#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxContext;

class HarmonyEmulatedTouchDispatcher {
 public:
  HarmonyEmulatedTouchDispatcher();
  ~HarmonyEmulatedTouchDispatcher();

  void EmulateTouch(const std::shared_ptr<LynxContext>& context,
                    const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count,
                    bool use_window_coordinates);

 private:
  void EmulateTouchOnUIThread(LynxContext* context,
                              const std::string& event_type, int x, int y,
                              const std::string& button, float delta_x,
                              float delta_y, bool use_window_coordinates);
  bool InjectTouch(LynxContext* context, int32_t action, int x, int y,
                   int64_t event_time_us, bool use_window_coordinates);
  bool ToEventPoint(LynxContext* context, int x, int y, int32_t* display_x,
                    int32_t* display_y, int32_t* window_x, int32_t* window_y);
  void CancelActiveTouch(LynxContext* context, int64_t event_time_us);

  bool emulated_touch_active_ = false;
  bool emulated_touch_uses_window_coordinates_ = false;
  int emulated_touch_x_ = 0;
  int emulated_touch_y_ = 0;
  std::shared_ptr<bool> alive_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_HARMONY_EMULATED_TOUCH_DISPATCHER_H_
