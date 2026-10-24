// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/clay_input_event_target.h"

#include <cmath>
#include <utility>

#include "base/include/timer/time_utils.h"

namespace lynx {
namespace embedder {
namespace {

// Native desktop devices and the legacy CDP adapters use non-negative IDs.
constexpr int32_t kSyntheticGestureMouseDevice = -1;
constexpr std::array<int32_t, 2> kSyntheticGestureTouchDevices = {-2, -3};

bool SameViewport(const ClayInputViewport& first,
                  const ClayInputViewport& second) {
  return first.width == second.width && first.height == second.height &&
         first.pixel_ratio == second.pixel_ratio;
}

bool IsValidPointer(const devtool::input::Pointer& pointer,
                    const ClayInputViewport& viewport) {
  return std::isfinite(pointer.x) && std::isfinite(pointer.y) &&
         pointer.x >= 0 && pointer.y >= 0 && pointer.x < viewport.width &&
         pointer.y < viewport.height;
}

}  // namespace

bool ClayInputViewport::IsValid() const {
  return std::isfinite(width) && width > 0 && std::isfinite(height) &&
         height > 0 && std::isfinite(pixel_ratio) && pixel_ratio > 0;
}

ClayInputEventTarget::ClayInputEventTarget(ViewportCallback viewport,
                                           SendCallback send)
    : viewport_(std::move(viewport)), send_(std::move(send)) {}

ClayInputEventTarget::~ClayInputEventTarget() { Invalidate(); }

devtool::input::PointerCapabilities
ClayInputEventTarget::GetPointerCapabilities() const {
  devtool::input::PointerCapabilities capabilities;
  if (viewport_ && send_ && viewport_().IsValid()) {
    capabilities.default_source_type =
        devtool::input::PointerSourceType::kMouse;
    capabilities.supports_touch = true;
    capabilities.supports_mouse = true;
    capabilities.max_touch_points = kMaxTouchPoints;
  }
  return capabilities;
}

bool ClayInputEventTarget::InjectPointerEvent(
    const devtool::input::PointerEvent& event) {
  using devtool::input::PointerSourceType;
  if (!viewport_ || !send_ || event.modifiers != 0 || event.click_count < 0 ||
      event.click_count > 1) {
    CancelAll();
    return false;
  }
  const auto viewport = viewport_();
  if (!viewport.IsValid() || ((mouse_active_ || ActiveTouchCount() > 0) &&
                              !SameViewport(viewport, active_viewport_))) {
    CancelAll();
    return false;
  }
  switch (event.source_type) {
    case PointerSourceType::kMouse:
      return InjectMouseEvent(event, viewport);
    case PointerSourceType::kTouch:
      return InjectTouchEvent(event, viewport);
    case PointerSourceType::kDefault:
      CancelAll();
      return false;
  }
  CancelAll();
  return false;
}

ClayPointerEvent ClayInputEventTarget::MakeClayEvent(
    const devtool::input::Pointer& pointer, const ClayInputViewport& viewport,
    ClayPointerDeviceKind device_kind, int32_t device_id,
    int64_t timestamp_us) const {
  ClayPointerEvent clay_event{};
  clay_event.struct_size = sizeof(clay_event);
  clay_event.device = device_id;
  clay_event.device_kind = device_kind;
  clay_event.x = static_cast<double>(pointer.x) * viewport.pixel_ratio;
  clay_event.y = static_cast<double>(pointer.y) * viewport.pixel_ratio;
  clay_event.timestamp =
      timestamp_us > 0 ? timestamp_us : base::CurrentTimeMicroseconds();
  return clay_event;
}

bool ClayInputEventTarget::InjectMouseEvent(
    const devtool::input::PointerEvent& event,
    const ClayInputViewport& viewport) {
  using devtool::input::PointerEventType;
  const auto* pointer = event.FindPointer(event.action_pointer_id);
  if (event.pointers.size() != 1 || !pointer || ActiveTouchCount() > 0) {
    CancelAll();
    return false;
  }
  if (event.type == PointerEventType::kCancel) {
    const bool matches = mouse_active_ && mouse_pointer_id_ == pointer->id;
    CancelMouse();
    return matches;
  }
  if (!IsValidPointer(*pointer, viewport)) {
    CancelAll();
    return false;
  }

  ClayPointerEvent clay_event =
      MakeClayEvent(*pointer, viewport, kClayPointerDeviceKindMouse,
                    kSyntheticGestureMouseDevice, event.timestamp_us);
  switch (event.type) {
    case PointerEventType::kDown:
      if (mouse_active_ || event.buttons != devtool::input::kPrimaryButton) {
        CancelAll();
        return false;
      }
      clay_event.phase = kClayPointerPhaseDown;
      clay_event.buttons = kClayPointerMouseButtonsMousePrimary;
      break;
    case PointerEventType::kMove:
    case PointerEventType::kUp:
      if (!mouse_active_ || mouse_pointer_id_ != pointer->id ||
          event.buttons != (event.type == PointerEventType::kMove
                                ? devtool::input::kPrimaryButton
                                : devtool::input::kNoButton)) {
        CancelAll();
        return false;
      }
      clay_event.phase = event.type == PointerEventType::kMove
                             ? kClayPointerPhaseMove
                             : kClayPointerPhaseUp;
      clay_event.buttons = event.buttons;
      break;
    default:
      CancelAll();
      return false;
  }

  if (!mouse_added_) {
    auto add = clay_event;
    add.phase = kClayPointerPhaseAdd;
    add.buttons = 0;
    send_(add);
    mouse_added_ = true;
    last_mouse_event_ = add;
  }
  send_(clay_event);
  last_mouse_event_ = clay_event;
  mouse_active_ = event.type != PointerEventType::kUp;
  mouse_pointer_id_ = pointer->id;
  active_viewport_ = viewport;
  return true;
}

bool ClayInputEventTarget::InjectTouchEvent(
    const devtool::input::PointerEvent& event,
    const ClayInputViewport& viewport) {
  using devtool::input::PointerEventType;
  if (event.pointers.empty() || event.pointers.size() > kMaxTouchPoints ||
      !event.FindPointer(event.action_pointer_id) || mouse_active_) {
    CancelAll();
    return false;
  }
  for (std::size_t index = 0; index < event.pointers.size(); ++index) {
    if (!IsValidPointer(event.pointers[index], viewport)) {
      CancelAll();
      return false;
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (event.pointers[index].id == event.pointers[previous].id) {
        CancelAll();
        return false;
      }
    }
  }

  if (event.type == PointerEventType::kCancel) {
    const bool matches = FindTouchPointer(event.action_pointer_id) &&
                         HasSameActiveTouchSet(event);
    CancelTouches();
    return matches;
  }

  const std::size_t active_count = ActiveTouchCount();
  if (event.type == PointerEventType::kDown) {
    if (event.buttons != devtool::input::kPrimaryButton ||
        event.pointers.size() != active_count + 1 ||
        FindTouchPointer(event.action_pointer_id)) {
      CancelAll();
      return false;
    }
    for (const auto& pointer : event.pointers) {
      if (pointer.id != event.action_pointer_id &&
          !FindTouchPointer(pointer.id)) {
        CancelAll();
        return false;
      }
    }
    if (mouse_added_) {
      RemoveMouse();
    }
    TouchPointerState* state = nullptr;
    for (std::size_t index = 0; index < touch_pointers_.size(); ++index) {
      if (!touch_pointers_[index].active) {
        state = &touch_pointers_[index];
        state->device_id = kSyntheticGestureTouchDevices[index];
        break;
      }
    }
    if (!state) {
      CancelAll();
      return false;
    }
    const auto* action = event.FindPointer(event.action_pointer_id);
    auto clay_event =
        MakeClayEvent(*action, viewport, kClayPointerDeviceKindTouch,
                      state->device_id, event.timestamp_us);
    auto add = clay_event;
    add.phase = kClayPointerPhaseAdd;
    send_(add);
    clay_event.phase = kClayPointerPhaseDown;
    send_(clay_event);
    state->active = true;
    state->pointer_id = action->id;
    state->last_event = clay_event;
    active_viewport_ = viewport;
    return true;
  }

  if (event.buttons != devtool::input::kNoButton ||
      !HasSameActiveTouchSet(event)) {
    CancelAll();
    return false;
  }
  if (event.type == PointerEventType::kMove) {
    for (auto& state : touch_pointers_) {
      if (!state.active) {
        continue;
      }
      const auto* pointer = event.FindPointer(state.pointer_id);
      auto clay_event =
          MakeClayEvent(*pointer, viewport, kClayPointerDeviceKindTouch,
                        state.device_id, event.timestamp_us);
      clay_event.phase = kClayPointerPhaseMove;
      send_(clay_event);
      state.last_event = clay_event;
    }
    return true;
  }
  if (event.type == PointerEventType::kUp) {
    auto* state = FindTouchPointer(event.action_pointer_id);
    if (!state) {
      CancelAll();
      return false;
    }
    const auto* pointer = event.FindPointer(state->pointer_id);
    auto clay_event =
        MakeClayEvent(*pointer, viewport, kClayPointerDeviceKindTouch,
                      state->device_id, event.timestamp_us);
    clay_event.phase = kClayPointerPhaseUp;
    send_(clay_event);
    state->last_event = clay_event;
    RemoveTouch(*state);
    if (ActiveTouchCount() == 0) {
      active_viewport_ = {};
    }
    return true;
  }
  CancelAll();
  return false;
}

ClayInputEventTarget::TouchPointerState* ClayInputEventTarget::FindTouchPointer(
    int32_t pointer_id) {
  for (auto& pointer : touch_pointers_) {
    if (pointer.active && pointer.pointer_id == pointer_id) {
      return &pointer;
    }
  }
  return nullptr;
}

const ClayInputEventTarget::TouchPointerState*
ClayInputEventTarget::FindTouchPointer(int32_t pointer_id) const {
  for (const auto& pointer : touch_pointers_) {
    if (pointer.active && pointer.pointer_id == pointer_id) {
      return &pointer;
    }
  }
  return nullptr;
}

std::size_t ClayInputEventTarget::ActiveTouchCount() const {
  std::size_t count = 0;
  for (const auto& pointer : touch_pointers_) {
    if (pointer.active) {
      ++count;
    }
  }
  return count;
}

bool ClayInputEventTarget::HasSameActiveTouchSet(
    const devtool::input::PointerEvent& event) const {
  if (event.pointers.size() != ActiveTouchCount()) {
    return false;
  }
  for (const auto& pointer : touch_pointers_) {
    if (pointer.active && !event.FindPointer(pointer.pointer_id)) {
      return false;
    }
  }
  return true;
}

void ClayInputEventTarget::RemoveMouse() {
  if (!mouse_added_) {
    return;
  }
  auto event = last_mouse_event_;
  event.phase = kClayPointerPhaseRemove;
  event.buttons = 0;
  event.timestamp = base::CurrentTimeMicroseconds();
  if (send_) {
    send_(event);
  }
  mouse_added_ = false;
  mouse_active_ = false;
}

void ClayInputEventTarget::CancelMouse() {
  if (mouse_active_) {
    auto event = last_mouse_event_;
    event.phase = kClayPointerPhaseCancel;
    event.buttons = 0;
    event.timestamp = base::CurrentTimeMicroseconds();
    if (send_) {
      send_(event);
    }
    mouse_active_ = false;
  }
  // Removing also clears Clay's persistent mouse device and hover state.
  RemoveMouse();
}

void ClayInputEventTarget::RemoveTouch(TouchPointerState& pointer) {
  auto event = pointer.last_event;
  event.phase = kClayPointerPhaseRemove;
  event.buttons = 0;
  event.timestamp = base::CurrentTimeMicroseconds();
  if (send_) {
    send_(event);
  }
  pointer = {};
}

void ClayInputEventTarget::CancelTouches() {
  for (auto& pointer : touch_pointers_) {
    if (!pointer.active) {
      continue;
    }
    auto event = pointer.last_event;
    event.phase = kClayPointerPhaseCancel;
    event.buttons = 0;
    event.timestamp = base::CurrentTimeMicroseconds();
    if (send_) {
      send_(event);
    }
    pointer.last_event = event;
    RemoveTouch(pointer);
  }
  active_viewport_ = {};
}

void ClayInputEventTarget::CancelAll() {
  CancelMouse();
  CancelTouches();
}

void ClayInputEventTarget::CancelActivePointers() { CancelAll(); }

void ClayInputEventTarget::Invalidate() {
  CancelAll();
  viewport_ = nullptr;
  send_ = nullptr;
}

}  // namespace embedder
}  // namespace lynx
