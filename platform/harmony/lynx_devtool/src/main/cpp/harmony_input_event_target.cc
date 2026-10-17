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
  CancelActivePointersLocked(NowUs());
}

input::PointerCapabilities HarmonyInputEventTarget::GetPointerCapabilities()
    const {
  input::PointerCapabilities capabilities;
  std::lock_guard<std::mutex> lock(mutex_);
  if (injector_ && injector_->IsAvailable() && window_info_.IsValid()) {
    capabilities.default_source_type = input::PointerSourceType::kTouch;
    capabilities.supports_touch = true;
    capabilities.max_touch_points = kMaxTouchPoints;
  }
  return capabilities;
}

bool HarmonyInputEventTarget::InjectPointerEvent(
    const input::PointerEvent& event) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!injector_ || !injector_->IsAvailable() || !window_info_.IsValid() ||
      event.source_type != input::PointerSourceType::kTouch ||
      event.pointers.empty() || event.pointers.size() > kMaxTouchPoints) {
    CancelActivePointersLocked(event.timestamp_us);
    return false;
  }

  HarmonyTouchEventAction action;
  if (!ToHarmonyTouchAction(event.type, action) ||
      !IsValidTransitionLocked(event, action)) {
    CancelActivePointersLocked(event.timestamp_us);
    return false;
  }

  std::array<ActivePointer, kMaxTouchPoints> pointers;
  if (!PreparePointersLocked(event, action, pointers)) {
    CancelActivePointersLocked(event.timestamp_us);
    return false;
  }

  const auto inject_pointer = [&](const ActivePointer& pointer) {
    return injector_->Inject(
        BuildTouchEventLocked(pointer, action, event.timestamp_us));
  };

  if (action == HarmonyTouchEventAction::kCancel) {
    bool injected = true;
    for (const auto& pointer : pointers) {
      if (pointer.active && !inject_pointer(pointer)) {
        injected = false;
      }
    }
    ResetActivePointersLocked();
    return injected;
  }

  if (action == HarmonyTouchEventAction::kMove) {
    for (const auto& pointer : pointers) {
      if (pointer.active && !inject_pointer(pointer)) {
        CancelActivePointersLocked(event.timestamp_us);
        return false;
      }
    }
    active_pointers_ = pointers;
    return true;
  }

  std::size_t action_pointer_index = kMaxTouchPoints;
  for (std::size_t index = 0; index < pointers.size(); ++index) {
    if (pointers[index].active &&
        pointers[index].pointer_id == event.action_pointer_id) {
      action_pointer_index = index;
      break;
    }
  }
  if (action_pointer_index == kMaxTouchPoints ||
      !inject_pointer(pointers[action_pointer_index])) {
    CancelActivePointersLocked(event.timestamp_us);
    return false;
  }

  active_pointers_ = pointers;
  if (action == HarmonyTouchEventAction::kUp) {
    active_pointers_[action_pointer_index] = ActivePointer();
  }
  return true;
}

void HarmonyInputEventTarget::UpdateWindowInfo(
    const HarmonyInputWindowInfo& window_info) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!SameWindowInfo(window_info_, window_info)) {
    CancelActivePointersLocked(NowUs());
  }
  window_info_ = window_info;
}

void HarmonyInputEventTarget::InvalidateWindow() {
  UpdateWindowInfo(HarmonyInputWindowInfo());
}

std::size_t HarmonyInputEventTarget::ActivePointerCountLocked() const {
  std::size_t count = 0;
  for (const auto& pointer : active_pointers_) {
    if (pointer.active) {
      ++count;
    }
  }
  return count;
}

bool HarmonyInputEventTarget::IsValidTransitionLocked(
    const input::PointerEvent& event, HarmonyTouchEventAction action) const {
  if (!event.FindPointer(event.action_pointer_id)) {
    return false;
  }
  for (std::size_t index = 0; index < event.pointers.size(); ++index) {
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (event.pointers[index].id == event.pointers[previous].id) {
        return false;
      }
    }
  }

  const std::size_t active_count = ActivePointerCountLocked();
  if (action == HarmonyTouchEventAction::kDown) {
    if (event.pointers.size() != active_count + 1) {
      return false;
    }
    for (const auto& active_pointer : active_pointers_) {
      if (!active_pointer.active) {
        continue;
      }
      if (active_pointer.pointer_id == event.action_pointer_id ||
          !SameWindowInfo(active_pointer.window_info, window_info_) ||
          !event.FindPointer(active_pointer.pointer_id)) {
        return false;
      }
    }
    return true;
  }

  if (active_count == 0 || event.pointers.size() != active_count) {
    return false;
  }
  for (const auto& active_pointer : active_pointers_) {
    if (active_pointer.active &&
        (!SameWindowInfo(active_pointer.window_info, window_info_) ||
         !event.FindPointer(active_pointer.pointer_id))) {
      return false;
    }
  }
  return true;
}

bool HarmonyInputEventTarget::PreparePointersLocked(
    const input::PointerEvent& event, HarmonyTouchEventAction action,
    std::array<ActivePointer, kMaxTouchPoints>& pointers) const {
  pointers = active_pointers_;
  if (action == HarmonyTouchEventAction::kDown) {
    for (std::size_t index = 0; index < pointers.size(); ++index) {
      if (!pointers[index].active) {
        pointers[index].active = true;
        pointers[index].pointer_id = event.action_pointer_id;
        pointers[index].platform_pointer_id = static_cast<int32_t>(index);
        break;
      }
    }
  }

  for (const auto& pointer : event.pointers) {
    ActivePointer* matching_pointer = nullptr;
    for (auto& active_pointer : pointers) {
      if (active_pointer.active && active_pointer.pointer_id == pointer.id) {
        matching_pointer = &active_pointer;
        break;
      }
    }
    if (!matching_pointer ||
        !UpdatePointerCoordinatesLocked(pointer, *matching_pointer)) {
      return false;
    }
  }
  return true;
}

bool HarmonyInputEventTarget::UpdatePointerCoordinatesLocked(
    const input::Pointer& pointer, ActivePointer& active_pointer) const {
  if (!std::isfinite(pointer.x) || !std::isfinite(pointer.y)) {
    return false;
  }
  int32_t window_x = 0;
  int32_t window_y = 0;
  if (!ToInt32Coordinate(
          static_cast<double>(pointer.x) * window_info_.pixel_ratio,
          window_x) ||
      !ToInt32Coordinate(
          static_cast<double>(pointer.y) * window_info_.pixel_ratio,
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

  active_pointer.window_x = window_x;
  active_pointer.window_y = window_y;
  active_pointer.display_x = display_x;
  active_pointer.display_y = display_y;
  active_pointer.window_info = window_info_;
  return true;
}

HarmonyTouchEvent HarmonyInputEventTarget::BuildTouchEventLocked(
    const ActivePointer& pointer, HarmonyTouchEventAction action,
    int64_t timestamp_us) const {
  HarmonyTouchEvent touch_event;
  touch_event.action = action;
  touch_event.window_id = pointer.window_info.window_id;
  touch_event.display_id = pointer.window_info.display_id;
  touch_event.pointer_id = pointer.platform_pointer_id;
  touch_event.window_x = pointer.window_x;
  touch_event.window_y = pointer.window_y;
  touch_event.display_x = pointer.display_x;
  touch_event.display_y = pointer.display_y;
  touch_event.timestamp_us = timestamp_us > 0 ? timestamp_us : NowUs();
  return touch_event;
}

bool HarmonyInputEventTarget::CancelActivePointersLocked(int64_t timestamp_us) {
  bool injected = true;
  for (const auto& pointer : active_pointers_) {
    if (pointer.active &&
        (!injector_ || !injector_->IsAvailable() ||
         !injector_->Inject(BuildTouchEventLocked(
             pointer, HarmonyTouchEventAction::kCancel, timestamp_us)))) {
      injected = false;
    }
  }
  ResetActivePointersLocked();
  return injected;
}

void HarmonyInputEventTarget::ResetActivePointersLocked() {
  active_pointers_ = {};
}

}  // namespace devtool
}  // namespace lynx
