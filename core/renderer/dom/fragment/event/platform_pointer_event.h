// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POINTER_EVENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POINTER_EVENT_H_

#include "core/renderer/dom/fragment/event/platform_input_event.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

class PlatformPointerEvent : public PlatformInputEvent {
  using PointerFloatVec = base::InlineVector<float, 4>;
  using PointerIntVec = base::InlineVector<int, 4>;

 public:
  PlatformPointerEvent(int int_event_data[], float float_event_data[]);

  int PointerCount() const { return pointer_count_; }
  const PointerIntVec& PointerID() const { return pointer_id_; }
  const PointerFloatVec& PointerX() const { return pointer_x_; }
  const PointerFloatVec& PointerY() const { return pointer_y_; }

 private:
  int pointer_count_{0};
  PointerIntVec pointer_id_;
  PointerFloatVec pointer_x_;
  PointerFloatVec pointer_y_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POINTER_EVENT_H_
