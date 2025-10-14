// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_EVENT_DELEGATE_H_
#define CLAY_PUBLIC_EVENT_DELEGATE_H_

#include <map>
#include <memory>
#include <string>

#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/public/value.h"

namespace clay {

class EventDelegate {
 public:
  virtual ~EventDelegate() {}

  virtual void OnTouchEvent(const std::string& event_name, int tag, float x,
                            float y, float page_x, float page_y) = 0;
  virtual void OnMouseEvent(const std::string& event_name, int view_id,
                            int button, int buttons, float scale, float x,
                            float y, float page_x, float page_y) = 0;
  virtual void OnWheelEvent(const std::string& event_name, int view_id, float x,
                            float y, float page_x, float page_y, float delta_x,
                            float delta_y) = 0;
  virtual void OnKeyEvent(const std::string& event_name, int view_id,
                          const char* key, bool repeat) = 0;
  virtual void OnAnimationEvent(const std::string& event_name,
                                const char* animation_name, int view_id) = 0;
  virtual void OnTransitionEvent(const std::string& event_name,
                                 const char* animation_name, int view_id,
                                 ClayAnimationPropertyType type) = 0;
  virtual void OnFocusChanged(int view_id, bool focus) = 0;
  virtual void OnHoverChanged(int view_id, bool hover) = 0;
  virtual void OnDragDropEvent(const std::string& event_name, int view_id,
                               clay::Value::Map map) = 0;
  virtual void OnViewportMetricsChanged(
      double device_pixel_ratio, double device_density_dpi,
      double logical_width, double logical_height, double physical_screen_width,
      double physical_screen_height, double font_scale, bool night_mode) = 0;
  virtual void OnDrawEndEvent() = 0;
  virtual void OnSendCustomEvent(int view_id, const std::string& event_name,
                                 clay::Value::Map args) = 0;
  virtual void OnSendGlobalEvent(const std::string& event_name,
                                 clay::Value args) = 0;
  virtual void OnFirstMeaningfulPaint() = 0;
  virtual void OnOverlayEvent(int view_id, const char* overlay_id,
                              int overlay_count, const char** overlay_ids,
                              const char* event_name) = 0;
  virtual void OnLayoutChanged(int view_id, clay::Value::Map map) = 0;
  virtual void OnIntersectionEvent(int view_id, clay::Value::Map map) = 0;
  virtual void OnCallJSApiCallback(int callback_id, clay::Value value) = 0;
  virtual void CallJSIntersectionObserver(int observer_id, int callback_id,
                                          clay::Value params) = 0;
};

}  // namespace clay

#endif  // CLAY_PUBLIC_EVENT_DELEGATE_H_
