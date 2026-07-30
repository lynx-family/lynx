// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/devtool/harmony_emulated_touch_dispatcher.h"

#include <dlfcn.h>
#include <multimodalinput/oh_input_manager.h>

#include <chrono>
#include <cmath>
#include <cstdint>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "platform/embedder/public/lynx_event_simulation_proxy.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

constexpr int32_t kEmulatedTouchPointerId = 0;
constexpr int32_t kDefaultDisplayId = 0;
constexpr const char* kOhInputSoName =
    "lib"
    "oh"
    "input.so";
constexpr const char* kNativeWindowManagerSoName =
    "libnative_window_manager.so";
constexpr const char* kInjectTouchEventSymbol =
    "OH_WindowManager_InjectTouchEvent";

using WindowInjectTouchEventFunc = int32_t (*)(int32_t, Input_TouchEvent*,
                                               int32_t, int32_t);
using InputCreateTouchEventFunc = Input_TouchEvent* (*)();
using InputDestroyTouchEventFunc = void (*)(Input_TouchEvent**);
using InputSetTouchEventInt32Func = void (*)(Input_TouchEvent*, int32_t);
using InputSetTouchEventInt64Func = void (*)(Input_TouchEvent*, int64_t);

struct OhInputTouchEventApi {
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
    return create && destroy && set_action && set_finger_id && set_display_id &&
           set_display_x && set_display_y && set_action_time && set_window_id;
  }
};

int64_t NowUs() {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

WindowInjectTouchEventFunc GetWindowInjectTouchEventFunc() {
  static WindowInjectTouchEventFunc func = []() -> WindowInjectTouchEventFunc {
    void* handle = dlopen(kNativeWindowManagerSoName, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
      LOGW("GetWindowInjectTouchEventFunc: failed to load "
           << kNativeWindowManagerSoName << ", error=" << dlerror());
      return nullptr;
    }
    auto* symbol = dlsym(handle, kInjectTouchEventSymbol);
    if (!symbol) {
      LOGW("GetWindowInjectTouchEventFunc: missing "
           << kInjectTouchEventSymbol);
      return nullptr;
    }
    return reinterpret_cast<WindowInjectTouchEventFunc>(symbol);
  }();
  return func;
}

OhInputTouchEventApi* GetOhInputTouchEventApi() {
  static OhInputTouchEventApi api = []() -> OhInputTouchEventApi {
    OhInputTouchEventApi api;
    void* handle = dlopen(kOhInputSoName, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
      LOGW("GetOhInputTouchEventApi: failed to load "
           << kOhInputSoName << ", error=" << dlerror());
      return api;
    }

    api.create = reinterpret_cast<InputCreateTouchEventFunc>(
        dlsym(handle, "OH_Input_CreateTouchEvent"));
    api.destroy = reinterpret_cast<InputDestroyTouchEventFunc>(
        dlsym(handle, "OH_Input_DestroyTouchEvent"));
    api.set_action = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventAction"));
    api.set_finger_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventFingerId"));
    api.set_display_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventDisplayId"));
    api.set_display_x = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventDisplayX"));
    api.set_display_y = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventDisplayY"));
    api.set_action_time = reinterpret_cast<InputSetTouchEventInt64Func>(
        dlsym(handle, "OH_Input_SetTouchEventActionTime"));
    api.set_window_id = reinterpret_cast<InputSetTouchEventInt32Func>(
        dlsym(handle, "OH_Input_SetTouchEventWindowId"));
    if (!api.IsValid()) {
      LOGW("GetOhInputTouchEventApi: missing required OH input symbol");
    }
    return api;
  }();
  return api.IsValid() ? &api : nullptr;
}

}  // namespace

HarmonyEmulatedTouchDispatcher::HarmonyEmulatedTouchDispatcher()
    : alive_(std::make_shared<bool>(true)) {}

HarmonyEmulatedTouchDispatcher::~HarmonyEmulatedTouchDispatcher() {
  if (alive_) {
    *alive_ = false;
  }
}

void HarmonyEmulatedTouchDispatcher::EmulateTouch(
    const std::shared_ptr<LynxContext>& context, const std::string& event_type,
    int x, int y, const std::string& button, float delta_x, float delta_y,
    int modifiers, int click_count, bool use_window_coordinates) {
  (void)modifiers;
  (void)click_count;
  if (!context) {
    LOGE("EmulateTouch: context is null");
    return;
  }
  context->RunOnUIThread([alive = alive_, context, event_type, x, y, button,
                          delta_x, delta_y, use_window_coordinates, this]() {
    if (!alive || !*alive) {
      return;
    }
    EmulateTouchOnUIThread(context.get(), event_type, x, y, button, delta_x,
                           delta_y, use_window_coordinates);
  });
}

void HarmonyEmulatedTouchDispatcher::EmulateTouchOnUIThread(
    LynxContext* context, const std::string& event_type, int x, int y,
    const std::string& button, float delta_x, float delta_y,
    bool use_window_coordinates) {
  (void)button;
  (void)delta_x;
  (void)delta_y;
  using lynx::pub::LynxEventSimulationProxy;
  const int64_t event_time_us = NowUs();

  if (event_type == LynxEventSimulationProxy::kMousePressed) {
    if (emulated_touch_active_) {
      CancelActiveTouch(context, event_time_us);
    }
    if (InjectTouch(context, TOUCH_ACTION_DOWN, x, y, event_time_us,
                    use_window_coordinates)) {
      emulated_touch_active_ = true;
      emulated_touch_uses_window_coordinates_ = use_window_coordinates;
      emulated_touch_x_ = x;
      emulated_touch_y_ = y;
    }
  } else if (event_type == LynxEventSimulationProxy::kMouseMoved) {
    if (!emulated_touch_active_) {
      return;
    }
    if (InjectTouch(context, TOUCH_ACTION_MOVE, x, y, event_time_us,
                    emulated_touch_uses_window_coordinates_)) {
      emulated_touch_x_ = x;
      emulated_touch_y_ = y;
    }
  } else if (event_type == LynxEventSimulationProxy::kMouseReleased) {
    if (!emulated_touch_active_) {
      return;
    }
    if (InjectTouch(context, TOUCH_ACTION_UP, x, y, event_time_us,
                    emulated_touch_uses_window_coordinates_)) {
      emulated_touch_active_ = false;
      emulated_touch_uses_window_coordinates_ = false;
      emulated_touch_x_ = x;
      emulated_touch_y_ = y;
    }
  }
}

bool HarmonyEmulatedTouchDispatcher::InjectTouch(LynxContext* context,
                                                 int32_t action, int x, int y,
                                                 int64_t event_time_us,
                                                 bool use_window_coordinates) {
  int32_t display_x = 0;
  int32_t display_y = 0;
  int32_t window_x = 0;
  int32_t window_y = 0;
  if (use_window_coordinates) {
    if (!context || !context->HasWindowInfo()) {
      LOGE("InjectTouch: window info is not available");
      return false;
    }
    window_x = x;
    window_y = y;
    display_x = context->WindowLeftPx() + x;
    display_y = context->WindowTopPx() + y;
  } else {
    if (!ToEventPoint(context, x, y, &display_x, &display_y, &window_x,
                      &window_y)) {
      return false;
    }
  }

  auto* input_api = GetOhInputTouchEventApi();
  auto* window_inject_func = GetWindowInjectTouchEventFunc();
  if (!input_api || !window_inject_func) {
    return false;
  }

  auto* event = input_api->create();
  if (!event) {
    LOGE("InjectTouch: failed to create touch event");
    return false;
  }
  input_api->set_action(event, action);
  input_api->set_finger_id(event, kEmulatedTouchPointerId);
  input_api->set_display_id(event, kDefaultDisplayId);
  input_api->set_display_x(event, display_x);
  input_api->set_display_y(event, display_y);
  input_api->set_action_time(event, event_time_us);

  bool injected = false;
  input_api->set_window_id(event, context->WindowId());
  const int32_t result =
      window_inject_func(context->WindowId(), event, window_x, window_y);
  if (result == 0) {
    injected = true;
  } else {
    LOGE("InjectTouch: OH_WindowManager_InjectTouchEvent failed with "
         << result << ", windowId=" << context->WindowId() << ", windowX="
         << window_x << ", windowY=" << window_y << ", displayX=" << display_x
         << ", displayY=" << display_y << ", x=" << x << ", y=" << y);
  }
  input_api->destroy(&event);
  return injected;
}

bool HarmonyEmulatedTouchDispatcher::ToEventPoint(LynxContext* context, int x,
                                                  int y, int32_t* display_x,
                                                  int32_t* display_y,
                                                  int32_t* window_x,
                                                  int32_t* window_y) {
  if (!context || !display_x || !display_y || !window_x || !window_y) {
    return false;
  }
  if (!context->HasWindowInfo()) {
    LOGE("ToEventPoint: window info is not available");
    return false;
  }
  UIRoot* root = context->Root();
  if (!root) {
    LOGE("ToEventPoint: root is null");
    return false;
  }
  float scaled_density = context->ScaledDensity();
  if (base::FloatsLargerOrEqual(0.f, scaled_density)) {
    scaled_density = 1.f;
  }
  float offset_screen[2] = {0.f, 0.f};
  root->GetOffsetToScreen(offset_screen);
  *display_x =
      static_cast<int32_t>(std::llround(offset_screen[0] * scaled_density + x));
  *display_y =
      static_cast<int32_t>(std::llround(offset_screen[1] * scaled_density + y));
  *window_x = *display_x - context->WindowLeftPx();
  *window_y = *display_y - context->WindowTopPx();
  return true;
}

void HarmonyEmulatedTouchDispatcher::CancelActiveTouch(LynxContext* context,
                                                       int64_t event_time_us) {
  if (!emulated_touch_active_) {
    return;
  }
  InjectTouch(context, TOUCH_ACTION_CANCEL, emulated_touch_x_,
              emulated_touch_y_, event_time_us,
              emulated_touch_uses_window_coordinates_);
  emulated_touch_active_ = false;
  emulated_touch_uses_window_coordinates_ = false;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
