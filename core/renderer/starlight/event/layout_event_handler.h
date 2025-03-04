// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_HANDLER_H_
#define CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_HANDLER_H_

#include "core/renderer/starlight/event/layout_event.h"
#include "core/renderer/starlight/event/layout_event_data.h"

namespace lynx {
namespace starlight {

class LayoutObject;

class LayoutEventHandler {
 public:
  virtual ~LayoutEventHandler() = default;
  virtual void OnLayoutEvent(const LayoutObject* node, LayoutEventType type,
                             const LayoutEventData& data) = 0;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_HANDLER_H_
