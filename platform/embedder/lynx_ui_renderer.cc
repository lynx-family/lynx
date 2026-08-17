// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_ui_renderer.h"

#include <string_view>

#include "base/include/timer/time_utils.h"

namespace lynx {
namespace embedder {

namespace {

constexpr int32_t kSyntheticMouseDeviceId = 0;
static constexpr const char* kMousePressed = "mousePressed";
static constexpr const char* kMouseMoved = "mouseMoved";
static constexpr const char* kMouseReleased = "mouseReleased";
static constexpr const char* kMouseWheel = "mouseWheel";
static constexpr const char* kMouseLeftButton = "left";
static constexpr const char* kMouseRightButton = "right";
static constexpr const char* kMouseMiddleButton = "middle";
static constexpr const char* kMouseBackButton = "back";
static constexpr const char* kMouseForwardButton = "forward";

int64_t ButtonMask(std::string_view button) {
  if (button == kMouseLeftButton) {
    return kClayPointerMouseButtonsMousePrimary;
  }
  if (button == kMouseRightButton) {
    return kClayPointerMouseButtonsMouseSecondary;
  }
  if (button == kMouseMiddleButton) {
    return kClayPointerMouseButtonsMouseMiddle;
  }
  if (button == kMouseBackButton) {
    return kClayPointerMouseButtonsMouseBack;
  }
  if (button == kMouseForwardButton) {
    return kClayPointerMouseButtonsMouseForward;
  }
  return 0;
}

}  // namespace

void LynxUIRenderer::DispatchSyntheticPointerEvent(
    const char* event_type, float x, float y, const char* button, float delta_x,
    float delta_y, int modifiers, int click_count) {
  // ClayPointerEvent has no modifier or click-count fields. Supporting them
  // requires separate key events or higher-level gesture handling; keep the
  // parameters for compatibility with the DevTool interface.
  (void)modifiers;
  (void)click_count;
  if (!event_type) {
    return;
  }

  const std::string_view type(event_type);
  if (type != kMousePressed && type != kMouseMoved && type != kMouseReleased &&
      type != kMouseWheel) {
    return;
  }

  ClayPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.x = x * pixel_ratio_;
  event.y = y * pixel_ratio_;
  event.device = kSyntheticMouseDeviceId;
  event.device_kind = kClayPointerDeviceKindMouse;

  const auto send_event = [this](ClayPointerEvent pointer_event) {
    pointer_event.timestamp = base::CurrentTimeMicroseconds();
    SendPointerEvent(pointer_event);
  };
  const auto send_lifecycle_event = [&event,
                                     &send_event](ClayPointerPhase phase) {
    ClayPointerEvent lifecycle_event = event;
    lifecycle_event.phase = phase;
    lifecycle_event.buttons = 0;
    lifecycle_event.signal_kind = kClayPointerSignalKindNone;
    lifecycle_event.scroll_delta_x = 0;
    lifecycle_event.scroll_delta_y = 0;
    lifecycle_event.is_precise_scroll = 0;
    send_event(lifecycle_event);
  };

  const int64_t button_mask = ButtonMask(button ? button : "");
  if (type == kMousePressed) {
    event.phase = kClayPointerPhaseDown;
    event.buttons =
        button_mask != 0 ? button_mask : kClayPointerMouseButtonsMousePrimary;
  } else if (type == kMouseReleased) {
    event.phase = kClayPointerPhaseUp;
    event.buttons = 0;
  } else if (type == kMouseMoved) {
    // Button state is supplied by the caller for every move. A zero mask is
    // therefore an intentional hover; this renderer keeps no drag state.
    event.phase =
        button_mask != 0 ? kClayPointerPhaseMove : kClayPointerPhaseHover;
    event.buttons = button_mask;
  } else {
    event.phase = kClayPointerPhaseHover;
    event.buttons = 0;
  }

  if (type == kMouseWheel) {
    event.signal_kind = kClayPointerSignalKindScroll;
    event.scroll_delta_x = delta_x * pixel_ratio_;
    event.scroll_delta_y = delta_y * pixel_ratio_;
    event.is_precise_scroll = 1;
  }

  // Platform mouse adapters normally add a pointer before dispatching input
  // and remove it when the pointer leaves. Synthetic events bypass those
  // adapters, so provide a complete Clay pointer lifecycle here. A dedicated
  // device ID keeps this lifecycle independent from the physical mouse.
  if (type == kMousePressed || type == kMouseWheel ||
      (type == kMouseMoved && button_mask == 0)) {
    send_lifecycle_event(kClayPointerPhaseAdd);
  }
  send_event(event);
  if (type == kMouseReleased || type == kMouseWheel ||
      (type == kMouseMoved && button_mask == 0)) {
    send_lifecycle_event(kClayPointerPhaseRemove);
  }
}

}  // namespace embedder
}  // namespace lynx
