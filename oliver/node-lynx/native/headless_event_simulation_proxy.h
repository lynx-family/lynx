// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef OLIVER_NODE_LYNX_NATIVE_HEADLESS_EVENT_SIMULATION_PROXY_H_
#define OLIVER_NODE_LYNX_NATIVE_HEADLESS_EVENT_SIMULATION_PROXY_H_

#include <functional>
#include <string>

#include "platform/embedder/public/capi/lynx_types.h"
#include "platform/embedder/public/lynx_event_simulation_proxy.h"

namespace lynx {
namespace node {

class HeadlessEventSimulationProxy final
    : public pub::LynxEventSimulationProxy {
 public:
  using DevicePixelRatioProvider = std::function<double()>;
  using PointerEventCallback = std::function<void(const lynx_pointer_event_t&)>;

  HeadlessEventSimulationProxy(
      DevicePixelRatioProvider device_pixel_ratio_provider,
      PointerEventCallback pointer_event_callback);

  void EmulateTouch(const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count) override;

 private:
  double DevicePixelRatio() const;
  void DispatchPointerEvent(
      lynx_pointer_phase_e phase, double x, double y,
      lynx_pointer_device_kind_e device_kind, int64_t buttons,
      lynx_pointer_signal_kind_e signal_kind = kLynxPointerSignalKindNone,
      double scroll_delta_x = 0, double scroll_delta_y = 0);

  DevicePixelRatioProvider device_pixel_ratio_provider_;
  PointerEventCallback pointer_event_callback_;
  bool touch_active_ = false;
  double last_x_ = 0;
  double last_y_ = 0;
};

}  // namespace node
}  // namespace lynx

#endif  // OLIVER_NODE_LYNX_NATIVE_HEADLESS_EVENT_SIMULATION_PROXY_H_
