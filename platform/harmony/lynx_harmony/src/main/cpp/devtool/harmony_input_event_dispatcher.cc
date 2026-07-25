// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/devtool/harmony_input_event_dispatcher.h"

#include <dlfcn.h>
#include <multimodalinput/oh_input_manager.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

constexpr int32_t kDefaultDisplayId = 0;
constexpr int32_t kPlatformPrimaryPointerId = 0;
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
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
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

bool ToInt32Coordinate(double value, int32_t* result) {
  if (!result || !std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
    return false;
  }
  *result = static_cast<int32_t>(std::llround(value));
  return true;
}

}  // namespace

HarmonyInputEventDispatcher::HarmonyInputEventDispatcher()
    : state_(std::make_shared<State>()) {}

HarmonyInputEventDispatcher::~HarmonyInputEventDispatcher() {
  if (state_) {
    state_->alive.store(false, std::memory_order_release);
  }
}

bool HarmonyInputEventDispatcher::InjectPointerEvent(
    const std::shared_ptr<LynxContext>& context,
    const input::PointerEvent& event) {
  if (!context || !context->GetUITaskRunner() ||
      event.source_type != input::PointerSourceType::kTouch ||
      event.type == input::PointerEventType::kScroll ||
      event.pointers.size() != 1 ||
      !event.FindPointer(event.changed_pointer_id) || !IsAvailable()) {
    return false;
  }

  context->RunOnUIThread([state = state_, context, event]() {
    if (!state || !state->alive.load(std::memory_order_acquire)) {
      return;
    }
    state->pending_injection_succeeded &=
        InjectPointerEventOnUIThread(context.get(), event, state.get());
  });
  return true;
}

void HarmonyInputEventDispatcher::WaitForInputProcessed(
    const std::shared_ptr<LynxContext>& context,
    std::function<void(bool)> callback) {
  if (!context) {
    callback(false);
    return;
  }
  const auto& ui_task_runner = context->GetUITaskRunner();
  if (!ui_task_runner) {
    callback(false);
    return;
  }
  ui_task_runner->PostTask(
      [state = state_, callback = std::move(callback)]() mutable {
        if (!state || !state->alive.load(std::memory_order_acquire)) {
          callback(false);
          return;
        }
        const bool succeeded = state->pending_injection_succeeded;
        state->pending_injection_succeeded = true;
        callback(succeeded);
      });
}

void HarmonyInputEventDispatcher::Shutdown(
    const std::shared_ptr<LynxContext>& context) {
  auto state = state_;
  if (!state || !state->alive.exchange(false, std::memory_order_acq_rel) ||
      !context || !context->GetUITaskRunner()) {
    return;
  }
  context->RunOnUIThread([state, context]() {
    CancelActiveTouch(context.get(), NowUs(), state.get());
  });
}

bool HarmonyInputEventDispatcher::IsAvailable() {
  return GetOhInputTouchEventApi() && GetWindowInjectTouchEventFunc();
}

bool HarmonyInputEventDispatcher::InjectPointerEventOnUIThread(
    LynxContext* context, const input::PointerEvent& event, State* state) {
  if (!state) {
    return false;
  }
  const auto* pointer = event.FindPointer(event.changed_pointer_id);
  if (!pointer) {
    return false;
  }
  const int64_t event_time_us =
      event.timestamp_us > 0 ? event.timestamp_us : NowUs();

  switch (event.type) {
    case input::PointerEventType::kDown: {
      if (state->touch_active) {
        return false;
      }
      const bool injected =
          InjectTouch(context, TOUCH_ACTION_DOWN, kPlatformPrimaryPointerId,
                      pointer->x, pointer->y, event_time_us);
      if (injected) {
        state->touch_active = true;
        state->active_pointer_id = pointer->id;
        state->active_touch_x = pointer->x;
        state->active_touch_y = pointer->y;
      }
      return injected;
    }
    case input::PointerEventType::kMove:
      if (!state->touch_active || pointer->id != state->active_pointer_id) {
        return false;
      }
      if (InjectTouch(context, TOUCH_ACTION_MOVE, kPlatformPrimaryPointerId,
                      pointer->x, pointer->y, event_time_us)) {
        state->active_touch_x = pointer->x;
        state->active_touch_y = pointer->y;
        return true;
      }
      return false;
    case input::PointerEventType::kUp:
      if (!state->touch_active || pointer->id != state->active_pointer_id) {
        return false;
      }
      if (InjectTouch(context, TOUCH_ACTION_UP, kPlatformPrimaryPointerId,
                      pointer->x, pointer->y, event_time_us)) {
        ResetActiveTouch(state);
        return true;
      }
      CancelActiveTouch(context, event_time_us, state);
      return false;
    case input::PointerEventType::kCancel:
      if (!state->touch_active || pointer->id != state->active_pointer_id) {
        return false;
      }
      return CancelActiveTouch(context, event_time_us, state);
    case input::PointerEventType::kScroll:
      return false;
  }
  return false;
}

bool HarmonyInputEventDispatcher::InjectTouch(LynxContext* context,
                                              int32_t action,
                                              int32_t pointer_id, float x,
                                              float y, int64_t event_time_us) {
  int32_t display_x = 0;
  int32_t display_y = 0;
  int32_t window_x = 0;
  int32_t window_y = 0;
  if (!ToEventPoint(context, x, y, &display_x, &display_y, &window_x,
                    &window_y)) {
    return false;
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
  input_api->set_finger_id(event, pointer_id);
  input_api->set_display_id(event, kDefaultDisplayId);
  input_api->set_display_x(event, display_x);
  input_api->set_display_y(event, display_y);
  input_api->set_action_time(event, event_time_us);
  input_api->set_window_id(event, context->WindowId());

  const int32_t result =
      window_inject_func(context->WindowId(), event, window_x, window_y);
  if (result != 0) {
    LOGE("InjectTouch: OH_WindowManager_InjectTouchEvent failed with "
         << result << ", windowId=" << context->WindowId() << ", windowX="
         << window_x << ", windowY=" << window_y << ", displayX=" << display_x
         << ", displayY=" << display_y << ", x=" << x << ", y=" << y);
  }
  input_api->destroy(&event);
  return result == 0;
}

bool HarmonyInputEventDispatcher::ToEventPoint(LynxContext* context, float x,
                                               float y, int32_t* display_x,
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
  if (!std::isfinite(x) || !std::isfinite(y) ||
      !std::isfinite(offset_screen[0]) || !std::isfinite(offset_screen[1]) ||
      !std::isfinite(scaled_density) ||
      !ToInt32Coordinate(
          (static_cast<double>(offset_screen[0]) + x) * scaled_density,
          display_x) ||
      !ToInt32Coordinate(
          (static_cast<double>(offset_screen[1]) + y) * scaled_density,
          display_y)) {
    return false;
  }
  return ToInt32Coordinate(
             static_cast<double>(*display_x) - context->WindowLeftPx(),
             window_x) &&
         ToInt32Coordinate(
             static_cast<double>(*display_y) - context->WindowTopPx(),
             window_y);
}

bool HarmonyInputEventDispatcher::CancelActiveTouch(LynxContext* context,
                                                    int64_t event_time_us,
                                                    State* state) {
  if (!state || !state->touch_active) {
    return true;
  }
  const bool injected =
      InjectTouch(context, TOUCH_ACTION_CANCEL, kPlatformPrimaryPointerId,
                  state->active_touch_x, state->active_touch_y, event_time_us);
  ResetActiveTouch(state);
  return injected;
}

void HarmonyInputEventDispatcher::ResetActiveTouch(State* state) {
  if (!state) {
    return;
  }
  state->touch_active = false;
  state->active_pointer_id = 0;
  state->active_touch_x = 0.f;
  state->active_touch_y = 0.f;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
