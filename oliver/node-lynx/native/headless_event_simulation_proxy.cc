// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "oliver/node-lynx/native/headless_event_simulation_proxy.h"

#include <chrono>
#include <utility>

namespace lynx {
namespace node {

namespace {

uint64_t NowMicros() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

}  // namespace

HeadlessEventSimulationProxy::HeadlessEventSimulationProxy(
    DevicePixelRatioProvider device_pixel_ratio_provider,
    PointerEventCallback pointer_event_callback)
    : device_pixel_ratio_provider_(std::move(device_pixel_ratio_provider)),
      pointer_event_callback_(std::move(pointer_event_callback)) {}

void HeadlessEventSimulationProxy::EmulateTouch(
    const std::string& event_type, int x, int y, const std::string& button,
    float delta_x, float delta_y, int modifiers, int click_count) {
  (void)modifiers;
  (void)click_count;

  const double device_pixel_ratio = DevicePixelRatio();
  const double event_x = static_cast<double>(x) * device_pixel_ratio;
  const double event_y = static_cast<double>(y) * device_pixel_ratio;

  if (event_type == kMousePressed) {
    if (button != kMouseLeftButton) {
      return;
    }
    if (touch_active_) {
      DispatchPointerEvent(kLynxPointerPhaseCancel, last_x_, last_y_,
                           kLynxPointerDeviceKindTouch, 0);
    }
    DispatchPointerEvent(kLynxPointerPhaseDown, event_x, event_y,
                         kLynxPointerDeviceKindTouch,
                         kLynxPointerMouseButtonsMousePrimary);
    touch_active_ = true;
    last_x_ = event_x;
    last_y_ = event_y;
  } else if (event_type == kMouseMoved) {
    if (!touch_active_) {
      return;
    }
    DispatchPointerEvent(kLynxPointerPhaseMove, event_x, event_y,
                         kLynxPointerDeviceKindTouch,
                         kLynxPointerMouseButtonsMousePrimary);
    last_x_ = event_x;
    last_y_ = event_y;
  } else if (event_type == kMouseReleased) {
    if (!touch_active_) {
      return;
    }
    DispatchPointerEvent(kLynxPointerPhaseUp, event_x, event_y,
                         kLynxPointerDeviceKindTouch, 0);
    touch_active_ = false;
    last_x_ = event_x;
    last_y_ = event_y;
  } else if (event_type == kMouseWheel) {
    DispatchPointerEvent(kLynxPointerPhaseHover, event_x, event_y,
                         kLynxPointerDeviceKindMouse, 0,
                         kLynxPointerSignalKindScroll,
                         static_cast<double>(delta_x) * device_pixel_ratio,
                         static_cast<double>(delta_y) * device_pixel_ratio);
  }
}

double HeadlessEventSimulationProxy::DevicePixelRatio() const {
  if (!device_pixel_ratio_provider_) {
    return 1.0;
  }
  const double device_pixel_ratio = device_pixel_ratio_provider_();
  return device_pixel_ratio > 0 ? device_pixel_ratio : 1.0;
}

void HeadlessEventSimulationProxy::DispatchPointerEvent(
    lynx_pointer_phase_e phase, double x, double y,
    lynx_pointer_device_kind_e device_kind, int64_t buttons,
    lynx_pointer_signal_kind_e signal_kind, double scroll_delta_x,
    double scroll_delta_y) {
  if (!pointer_event_callback_) {
    return;
  }
  lynx_pointer_event_t pointer_event{};
  pointer_event.struct_size = sizeof(pointer_event);
  pointer_event.phase = phase;
  pointer_event.timestamp = NowMicros();
  pointer_event.x = x;
  pointer_event.y = y;
  pointer_event.device = device_kind == kLynxPointerDeviceKindTouch ? 0 : 1;
  pointer_event.signal_kind = signal_kind;
  pointer_event.scroll_delta_x = scroll_delta_x;
  pointer_event.scroll_delta_y = scroll_delta_y;
  pointer_event.device_kind = device_kind;
  pointer_event.buttons = buttons;
  pointer_event.scale = 1.0;
  pointer_event_callback_(pointer_event);
}

}  // namespace node
}  // namespace lynx
