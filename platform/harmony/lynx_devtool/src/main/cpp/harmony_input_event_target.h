// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_INPUT_EVENT_TARGET_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_INPUT_EVENT_TARGET_H_

#include <cstdint>
#include <memory>
#include <mutex>

#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {

enum class HarmonyTouchEventAction {
  kCancel,
  kDown,
  kMove,
  kUp,
};

struct HarmonyInputWindowInfo {
  int32_t window_id = -1;
  int32_t display_id = 0;
  int32_t left_px = 0;
  int32_t top_px = 0;
  int32_t width_px = 0;
  int32_t height_px = 0;
  float pixel_ratio = 0.f;

  bool IsValid() const;
};

struct HarmonyTouchEvent {
  HarmonyTouchEventAction action = HarmonyTouchEventAction::kCancel;
  int32_t window_id = -1;
  int32_t display_id = 0;
  int32_t pointer_id = 0;
  int32_t window_x = 0;
  int32_t window_y = 0;
  int32_t display_x = 0;
  int32_t display_y = 0;
  int64_t timestamp_us = 0;
};

class HarmonyTouchEventInjector {
 public:
  virtual ~HarmonyTouchEventInjector() = default;

  virtual bool IsAvailable() const = 0;
  virtual bool Inject(const HarmonyTouchEvent& event) = 0;
};

std::shared_ptr<HarmonyTouchEventInjector> CreateHarmonyTouchEventInjector();

class HarmonyInputEventTarget final : public input::InputEventTarget {
 public:
  explicit HarmonyInputEventTarget(
      std::shared_ptr<HarmonyTouchEventInjector> injector);
  ~HarmonyInputEventTarget() override;

  input::PointerCapabilities GetPointerCapabilities() const override;
  bool InjectPointerEvent(const input::PointerEvent& event) override;

  void UpdateWindowInfo(const HarmonyInputWindowInfo& window_info);
  void InvalidateWindow();

 private:
  struct ActivePointer {
    bool active = false;
    int32_t pointer_id = 0;
    int32_t window_x = 0;
    int32_t window_y = 0;
    int32_t display_x = 0;
    int32_t display_y = 0;
    HarmonyInputWindowInfo window_info;
  };

  bool BuildTouchEventLocked(const input::PointerEvent& event,
                             HarmonyTouchEventAction action,
                             HarmonyTouchEvent& touch_event);
  bool CancelActivePointerLocked(int64_t timestamp_us);
  void ResetActivePointerLocked();

  std::shared_ptr<HarmonyTouchEventInjector> injector_;
  mutable std::mutex mutex_;
  HarmonyInputWindowInfo window_info_;
  ActivePointer active_pointer_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_HARMONY_INPUT_EVENT_TARGET_H_
