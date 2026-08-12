// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/harmony_input_event_target.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace devtool {
namespace {

constexpr int32_t kPrimaryTouchPointerId = 0;

int64_t NowUs() {
  return fml::TimePoint::Now().ToEpochDelta().ToMicroseconds();
}

bool ToInt32Coordinate(double value, int32_t& result) {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
    return false;
  }
  result = static_cast<int32_t>(std::llround(value));
  return true;
}

bool SameWindowInfo(const HarmonyInputWindowInfo& first,
                    const HarmonyInputWindowInfo& second) {
  return first.window_id == second.window_id &&
         first.display_id == second.display_id &&
         first.left_px == second.left_px && first.top_px == second.top_px &&
         first.width_px == second.width_px &&
         first.height_px == second.height_px &&
         first.pixel_ratio == second.pixel_ratio;
}

bool ToHarmonyTouchAction(input::PointerEventType type,
                          HarmonyTouchEventAction& action) {
  switch (type) {
    case input::PointerEventType::kDown:
      action = HarmonyTouchEventAction::kDown;
      return true;
    case input::PointerEventType::kMove:
      action = HarmonyTouchEventAction::kMove;
      return true;
    case input::PointerEventType::kUp:
      action = HarmonyTouchEventAction::kUp;
      return true;
    case input::PointerEventType::kCancel:
      action = HarmonyTouchEventAction::kCancel;
      return true;
    case input::PointerEventType::kScroll:
      return false;
  }
  return false;
}

}  // namespace

bool HarmonyInputWindowInfo::IsValid() const {
  return window_id >= 0 && width_px > 0 && height_px > 0 &&
         std::isfinite(pixel_ratio) && pixel_ratio > 0.f;
}

HarmonyInputEventTarget::HarmonyInputEventTarget(
    std::shared_ptr<HarmonyTouchEventInjector> injector)
    : injector_(std::move(injector)) {}

HarmonyInputEventTarget::~HarmonyInputEventTarget() {
  std::lock_guard<std::mutex> lock(mutex_);
  CancelActivePointerLocked(NowUs());
}

input::PointerCapabilities HarmonyInputEventTarget::GetPointerCapabilities()
    const {
  input::PointerCapabilities capabilities;
  std::lock_guard<std::mutex> lock(mutex_);
  if (injector_ && injector_->IsAvailable() && window_info_.IsValid()) {
    capabilities.default_source_type = input::PointerSourceType::kTouch;
    capabilities.supports_touch = true;
  }
  return capabilities;
}

bool HarmonyInputEventTarget::InjectPointerEvent(
    const input::PointerEvent& event) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!injector_ || !injector_->IsAvailable() || !window_info_.IsValid() ||
      event.source_type != input::PointerSourceType::kTouch ||
      event.pointers.size() != 1) {
    CancelActivePointerLocked(event.timestamp_us);
    return false;
  }

  HarmonyTouchEventAction action;
  const input::Pointer* pointer = event.FindPointer(event.action_pointer_id);
  if (!pointer || !std::isfinite(pointer->x) || !std::isfinite(pointer->y) ||
      !ToHarmonyTouchAction(event.type, action)) {
    CancelActivePointerLocked(event.timestamp_us);
    return false;
  }

  if (action == HarmonyTouchEventAction::kDown) {
    if (active_pointer_.active) {
      CancelActivePointerLocked(event.timestamp_us);
      return false;
    }
  } else if (!active_pointer_.active ||
             active_pointer_.pointer_id != pointer->id ||
             !SameWindowInfo(active_pointer_.window_info, window_info_)) {
    CancelActivePointerLocked(event.timestamp_us);
    return false;
  }

  HarmonyTouchEvent touch_event;
  if (!BuildTouchEventLocked(event, action, touch_event)) {
    CancelActivePointerLocked(event.timestamp_us);
    return false;
  }

  if (action == HarmonyTouchEventAction::kCancel) {
    const bool injected = injector_->Inject(touch_event);
    ResetActivePointerLocked();
    return injected;
  }

  if (!injector_->Inject(touch_event)) {
    if (action != HarmonyTouchEventAction::kDown) {
      CancelActivePointerLocked(event.timestamp_us);
    }
    return false;
  }

  if (action == HarmonyTouchEventAction::kDown) {
    active_pointer_.active = true;
    active_pointer_.pointer_id = pointer->id;
    active_pointer_.window_info = window_info_;
  }
  active_pointer_.window_x = touch_event.window_x;
  active_pointer_.window_y = touch_event.window_y;
  active_pointer_.display_x = touch_event.display_x;
  active_pointer_.display_y = touch_event.display_y;
  if (action == HarmonyTouchEventAction::kUp) {
    ResetActivePointerLocked();
  }
  return true;
}

void HarmonyInputEventTarget::UpdateWindowInfo(
    const HarmonyInputWindowInfo& window_info) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!SameWindowInfo(window_info_, window_info)) {
    CancelActivePointerLocked(NowUs());
  }
  window_info_ = window_info;
}

void HarmonyInputEventTarget::InvalidateWindow() {
  UpdateWindowInfo(HarmonyInputWindowInfo());
}

bool HarmonyInputEventTarget::BuildTouchEventLocked(
    const input::PointerEvent& event, HarmonyTouchEventAction action,
    HarmonyTouchEvent& touch_event) {
  const input::Pointer* pointer = event.FindPointer(event.action_pointer_id);
  if (!pointer) {
    return false;
  }

  int32_t window_x = 0;
  int32_t window_y = 0;
  if (!ToInt32Coordinate(
          static_cast<double>(pointer->x) * window_info_.pixel_ratio,
          window_x) ||
      !ToInt32Coordinate(
          static_cast<double>(pointer->y) * window_info_.pixel_ratio,
          window_y) ||
      window_x < 0 || window_y < 0 || window_x >= window_info_.width_px ||
      window_y >= window_info_.height_px) {
    return false;
  }

  int32_t display_x = 0;
  int32_t display_y = 0;
  if (!ToInt32Coordinate(static_cast<double>(window_info_.left_px) + window_x,
                         display_x) ||
      !ToInt32Coordinate(static_cast<double>(window_info_.top_px) + window_y,
                         display_y)) {
    return false;
  }

  touch_event.action = action;
  touch_event.window_id = window_info_.window_id;
  touch_event.display_id = window_info_.display_id;
  // Harmony treats touch pointer id 0 as the primary input. Synthetic gestures
  // use ids in [1, 31] for cross-platform uniqueness, so normalize the platform
  // finger id for this single-touch adapter.
  touch_event.pointer_id = kPrimaryTouchPointerId;
  touch_event.window_x = window_x;
  touch_event.window_y = window_y;
  touch_event.display_x = display_x;
  touch_event.display_y = display_y;
  touch_event.timestamp_us =
      event.timestamp_us > 0 ? event.timestamp_us : NowUs();
  return true;
}

bool HarmonyInputEventTarget::CancelActivePointerLocked(int64_t timestamp_us) {
  if (!active_pointer_.active) {
    return true;
  }
  HarmonyTouchEvent cancel_event;
  cancel_event.action = HarmonyTouchEventAction::kCancel;
  cancel_event.window_id = active_pointer_.window_info.window_id;
  cancel_event.display_id = active_pointer_.window_info.display_id;
  cancel_event.pointer_id = kPrimaryTouchPointerId;
  cancel_event.window_x = active_pointer_.window_x;
  cancel_event.window_y = active_pointer_.window_y;
  cancel_event.display_x = active_pointer_.display_x;
  cancel_event.display_y = active_pointer_.display_y;
  cancel_event.timestamp_us = timestamp_us > 0 ? timestamp_us : NowUs();
  const bool injected =
      injector_ && injector_->IsAvailable() && injector_->Inject(cancel_event);
  ResetActivePointerLocked();
  return injected;
}

void HarmonyInputEventTarget::ResetActivePointerLocked() {
  active_pointer_ = ActivePointer();
}

}  // namespace devtool
}  // namespace lynx
