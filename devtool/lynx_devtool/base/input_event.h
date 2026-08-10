// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_BASE_INPUT_EVENT_H_
#define DEVTOOL_LYNX_DEVTOOL_BASE_INPUT_EVENT_H_

#include <cstdint>
#include <vector>

namespace lynx {
namespace devtool {
namespace input {

enum class PointerSourceType {
  kDefault,
  kTouch,
  kMouse,
};

enum class PointerEventType {
  kDown,
  kMove,
  kUp,
  kCancel,
  kScroll,
};

enum PointerButton : int64_t {
  kNoButton = 0,
  kPrimaryButton = 1 << 0,
  kSecondaryButton = 1 << 1,
  kMiddleButton = 1 << 2,
  kBackButton = 1 << 3,
  kForwardButton = 1 << 4,
};

struct Pointer {
  int32_t id = 0;
  float x = 0.f;
  float y = 0.f;
  float radius_x = 1.f;
  float radius_y = 1.f;
  float rotation_angle = 0.f;
  float force = 1.f;
  float tangential_pressure = 0.f;
  float tilt_x = 0.f;
  float tilt_y = 0.f;
  int32_t twist = 0;
};

struct PointerEvent {
  PointerSourceType source_type = PointerSourceType::kDefault;
  PointerEventType type = PointerEventType::kMove;
  std::vector<Pointer> pointers;
  int32_t action_pointer_id = 0;
  float delta_x = 0.f;
  float delta_y = 0.f;
  int32_t modifiers = 0;
  int64_t buttons = kNoButton;
  int32_t click_count = 0;
  int64_t timestamp_us = 0;

  const Pointer* FindPointer(int32_t pointer_id) const {
    for (const auto& pointer : pointers) {
      if (pointer.id == pointer_id) {
        return &pointer;
      }
    }
    return nullptr;
  }
};

struct PointerCapabilities {
  PointerSourceType default_source_type = PointerSourceType::kDefault;
  bool supports_touch = false;
  bool supports_mouse = false;

  bool Supports(PointerSourceType source_type) const {
    switch (source_type) {
      case PointerSourceType::kTouch:
        return supports_touch;
      case PointerSourceType::kMouse:
        return supports_mouse;
      case PointerSourceType::kDefault:
        return false;
    }
    return false;
  }
};

int32_t GenerateSyntheticPointerId();

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_BASE_INPUT_EVENT_H_
