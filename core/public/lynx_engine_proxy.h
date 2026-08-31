// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_ENGINE_PROXY_H_
#define CORE_PUBLIC_LYNX_ENGINE_PROXY_H_

#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "base/include/closure.h"
#include "core/public/event/touch_event_data.h"
#include "core/public/external_memory_snapshot.h"
#include "core/public/list_data.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace shell {

struct CustomEventDispatchOptions {
  // Emergency tasks are selected before normal ready tasks at the next
  // message-loop boundary. They never preempt a task that is already running.
  bool emergency{false};
};

class LynxEngineProxy {
 public:
  virtual ~LynxEngineProxy() = default;

  virtual void DispatchTaskToLynxEngine(base::closure task) = 0;

  // Forwards a platform UI memory snapshot to the engine thread.
  virtual void ReportExternalMemory(
      tasm::ExternalMemorySnapshot /* snapshot */) {}

  // Event
  virtual bool SendTouchEvent(
      const std::string& name, int32_t tag, float x, float y, float client_x,
      float client_y, float page_x, float page_y, int64_t timestamp = 0,
      event::TouchEventTargetPoints current_target_points = {}) = 0;

  virtual bool SendTouchEvent(const std::string& name, const pub::Value& params,
                              int64_t timestamp = 0) = 0;

  virtual void SendCustomEvent(const std::string& name, int32_t tag,
                               const pub::Value& params,
                               const std::string& params_name) = 0;

  virtual void SendCustomEventWithOptions(
      const std::string& name, int32_t tag, const pub::Value& params,
      const std::string& params_name,
      const CustomEventDispatchOptions& options) {
    SendCustomEvent(name, tag, params, params_name);
  }

  virtual void SendGestureEvent(int tag, int gesture_id, std::string name,
                                const pub::Value& params) = 0;

  virtual void SendBubbleEvent(const std::string& name, int32_t tag,
                               const pub::Value& dict) = 0;

  virtual void OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                                     int32_t current_status) = 0;

  virtual void StartEventGenerate(const pub::Value& event_params) = 0;

  virtual void StartEventCapture(int64_t event_id) = 0;

  virtual void StartEventBubble(int64_t event_id) = 0;

  virtual void StartEventFire(bool is_stop, int64_t event_id) = 0;

  // List
  // TODO(chenyouhui): Split the list interface into its own public API.
  virtual void ScrollByListContainer(int32_t tag, float x, float y,
                                     float original_x, float original_y) = 0;

  virtual void ScrollToPosition(int32_t tag, int index, float offset, int align,
                                bool smooth) = 0;

  virtual void ScrollStopped(int32_t tag) = 0;

  virtual int32_t ObtainListChild(int32_t tag, uint32_t index,
                                  int64_t operation_id,
                                  bool enable_reuse_notification) = 0;

  virtual void RecycleListChild(int32_t tag, uint32_t sign) = 0;

  virtual void RenderListChild(int32_t tag, uint32_t index,
                               int64_t operation_id) = 0;

  virtual void UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                               int64_t operation_id) = 0;

  virtual tasm::ListData GetListData(int view_id) = 0;

  // This function constructs and returns a list of element tags synchronized
  // that represent the specified element and its ancestor elements in the DOM
  // hierarchy. The elements should not affected by the z-index attribute.
  // Do not call this function when using async tasm.
  virtual std::list<int32_t> GetAncestorElements(int32_t tag) = 0;

  virtual void MarkLayoutDirty(int sign) = 0;

  // Animation
  virtual bool EnableRasterAnimation() = 0;

  virtual float GetDensity() const = 0;

  virtual void OnFirstMeaningfulPaint() = 0;

  virtual void TriggerLayout() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_ENGINE_PROXY_H_
