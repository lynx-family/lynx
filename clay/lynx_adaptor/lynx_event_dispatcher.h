// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_LYNX_EVENT_DISPATCHER_H_
#define CLAY_LYNX_ADAPTOR_LYNX_EVENT_DISPATCHER_H_

#include <memory>
#include <string>

#include "clay/lynx_adaptor/perf_controller_clay.h"
#include "clay/public/event_delegate.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/perf_controller_proxy.h"

namespace clay {

class LynxEventDispatcher : public EventDelegate {
 public:
  LynxEventDispatcher() = default;
  ~LynxEventDispatcher() = default;

  LynxEventDispatcher(const LynxEventDispatcher&) = delete;
  LynxEventDispatcher& operator=(const LynxEventDispatcher&) = delete;

  void SetEngineProxy(
      const std::shared_ptr<lynx::shell::LynxEngineProxy>& engine_proxy) {
    engine_proxy_ = engine_proxy;
  }
  void SetRuntimeProxy(
      const std::shared_ptr<lynx::shell::LynxRuntimeProxy>& runtime_proxy) {
    runtime_proxy_ = runtime_proxy;
  }

  void SetPerfController(
      const std::shared_ptr<lynx::tasm::PerfControllerClay>& controller) {
    perf_controller_ = controller;
  }

  // clay::EventDelegate override
  void OnTouchEvent(const std::string& event_name, int tag, float x, float y,
                    float page_x, float page_y) override;
  void OnMouseEvent(const std::string& event_name, int view_id, int button,
                    int buttons, float scale, float x, float y, float page_x,
                    float page_y) override;
  void OnWheelEvent(const std::string& event_name, int view_id, float x,
                    float y, float page_x, float page_y, float delta_x,
                    float delta_y) override;
  void OnKeyEvent(const std::string& event_name, int view_id, const char* key,
                  bool repeat) override;

  void OnAnimationEvent(const std::string& event_name,
                        const char* animation_name, int view_id) override;
  void OnTransitionEvent(const std::string& event_name,
                         const char* animation_name, int view_id,
                         ClayAnimationPropertyType type) override;
  void OnFocusChanged(int view_id, bool focus) override;
  void OnHoverChanged(int view_id, bool hover) override;
  void OnDragDropEvent(const std::string& event_name, int view_id,
                       clay::Value::Map map) override;
  void OnViewportMetricsChanged(double device_pixel_ratio,
                                double device_density_dpi, double logical_width,
                                double logical_height,
                                double physical_screen_width,
                                double physical_screen_height,
                                double font_scale, bool night_mode) override {}

  void OnSendCustomEvent(int view_id, const std::string& event_name,
                         clay::Value::Map args) override;
  void OnSendGlobalEvent(const std::string& event_name,
                         clay::Value args) override;

  void OnDrawEndEvent() override;
  void OnFirstMeaningfulPaint() override;
  void OnOverlayEvent(int view_id, const char* overlay_id, int overlay_count,
                      const char** overlay_ids,
                      const char* event_name) override;
  void OnLayoutChanged(int view_id, clay::Value::Map map) override;
  void OnIntersectionEvent(int view_id, clay::Value::Map map) override;
  void OnCallJSApiCallback(int callback_id, clay::Value value) override;

  void CallJSIntersectionObserver(int observer_id, int callback_id,
                                  clay::Value params) override;

 private:
  std::shared_ptr<lynx::shell::LynxEngineProxy> engine_proxy_ = nullptr;
  std::shared_ptr<lynx::shell::LynxRuntimeProxy> runtime_proxy_ = nullptr;
  std::shared_ptr<lynx::tasm::PerfControllerClay> perf_controller_;
};

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_LYNX_EVENT_DISPATCHER_H_
