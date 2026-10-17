// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>
#include <multimodalinput/oh_input_manager.h>

#include <cstdint>
#include <memory>

#include "base/include/log/logging.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_input_event_target.h"

namespace lynx {
namespace devtool {
namespace {

constexpr const char* kOhInputSoName =
    "lib"
    "oh"
    "input.so";
constexpr const char* kNativeWindowManagerSoName =
    "libnative_window_manager.so";

using WindowInjectTouchEventFunc = int32_t (*)(int32_t, Input_TouchEvent*,
                                               int32_t, int32_t);
using InputCreateTouchEventFunc = Input_TouchEvent* (*)();
using InputDestroyTouchEventFunc = void (*)(Input_TouchEvent**);
using InputSetTouchEventInt32Func = void (*)(Input_TouchEvent*, int32_t);
using InputSetTouchEventInt64Func = void (*)(Input_TouchEvent*, int64_t);

struct HarmonyTouchApi {
  WindowInjectTouchEventFunc inject = nullptr;
  InputCreateTouchEventFunc create = nullptr;
  InputDestroyTouchEventFunc destroy = nullptr;
  InputSetTouchEventInt32Func set_action = nullptr;
  InputSetTouchEventInt32Func set_finger_id = nullptr;
  InputSetTouchEventInt32Func set_display_id = nullptr;
  InputSetTouchEventInt32Func set_display_x = nullptr;
  InputSetTouchEventInt32Func set_display_y = nullptr;
  InputSetTouchEventInt64Func set_action_time = nullptr;
  InputSetTouchEventInt32Func set_window_id = nullptr;

  bool IsValid() const {
    return inject && create && destroy && set_action && set_finger_id &&
           set_display_id && set_display_x && set_display_y &&
           set_action_time && set_window_id;
  }
};

void* LoadSymbol(void* handle, const char* name) {
  if (!handle) {
    return nullptr;
  }
  void* symbol = dlsym(handle, name);
  if (!symbol) {
    LOGW("HarmonyTouchEventInjector: missing " << name);
  }
  return symbol;
}

const HarmonyTouchApi& GetHarmonyTouchApi() {
  static const HarmonyTouchApi api = []() {
    HarmonyTouchApi result;
    void* input_handle = dlopen(kOhInputSoName, RTLD_NOW | RTLD_LOCAL);
    void* window_handle =
        dlopen(kNativeWindowManagerSoName, RTLD_NOW | RTLD_LOCAL);
    if (!input_handle || !window_handle) {
      LOGW("HarmonyTouchEventInjector: native input APIs are unavailable");
      return result;
    }

    result.inject = reinterpret_cast<WindowInjectTouchEventFunc>(
        LoadSymbol(window_handle, "OH_WindowManager_InjectTouchEvent"));
    result.create = reinterpret_cast<InputCreateTouchEventFunc>(
        LoadSymbol(input_handle, "OH_Input_CreateTouchEvent"));
    result.destroy = reinterpret_cast<InputDestroyTouchEventFunc>(
        LoadSymbol(input_handle, "OH_Input_DestroyTouchEvent"));
    result.set_action = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventAction"));
    result.set_finger_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventFingerId"));
    result.set_display_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventDisplayId"));
    result.set_display_x = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventDisplayX"));
    result.set_display_y = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventDisplayY"));
    result.set_action_time = reinterpret_cast<InputSetTouchEventInt64Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventActionTime"));
    result.set_window_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        LoadSymbol(input_handle, "OH_Input_SetTouchEventWindowId"));
    return result;
  }();
  return api;
}

int32_t ToPlatformAction(HarmonyTouchEventAction action) {
  switch (action) {
    case HarmonyTouchEventAction::kCancel:
      return TOUCH_ACTION_CANCEL;
    case HarmonyTouchEventAction::kDown:
      return TOUCH_ACTION_DOWN;
    case HarmonyTouchEventAction::kMove:
      return TOUCH_ACTION_MOVE;
    case HarmonyTouchEventAction::kUp:
      return TOUCH_ACTION_UP;
  }
  return TOUCH_ACTION_CANCEL;
}

class HarmonyTouchEventInjectorImpl final : public HarmonyTouchEventInjector {
 public:
  bool IsAvailable() const override { return GetHarmonyTouchApi().IsValid(); }

  bool Inject(const HarmonyTouchEvent& event) override {
    const auto& api = GetHarmonyTouchApi();
    if (!api.IsValid()) {
      return false;
    }
    Input_TouchEvent* touch_event = api.create();
    if (!touch_event) {
      return false;
    }

    api.set_action(touch_event, ToPlatformAction(event.action));
    api.set_finger_id(touch_event, event.pointer_id);
    api.set_display_id(touch_event, event.display_id);
    api.set_display_x(touch_event, event.display_x);
    api.set_display_y(touch_event, event.display_y);
    api.set_action_time(touch_event, event.timestamp_us);
    api.set_window_id(touch_event, event.window_id);
    const int32_t result = api.inject(event.window_id, touch_event,
                                      event.window_x, event.window_y);
    api.destroy(&touch_event);
    if (result != 0) {
      LOGE("HarmonyTouchEventInjector: injection failed with "
           << result << ", windowId=" << event.window_id
           << ", windowX=" << event.window_x << ", windowY=" << event.window_y
           << ", displayX=" << event.display_x
           << ", displayY=" << event.display_y);
    }
    return result == 0;
  }
};

}  // namespace

std::shared_ptr<HarmonyTouchEventInjector> CreateHarmonyTouchEventInjector() {
  return std::make_shared<HarmonyTouchEventInjectorImpl>();
}

}  // namespace devtool
}  // namespace lynx
