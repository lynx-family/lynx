// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_emitter.h"

#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

void EventEmitter::SendEvent(const LynxEvent& event) const {
  switch (event.EventType()) {
    case LynxEventType::kTouch: {
      const TouchEvent& touch_event = static_cast<const TouchEvent&>(event);
      if (touch_event.IsMultiTouch()) {
        ui_owner_->HandleMultiTouchEvent(touch_event);
      } else {
        ui_owner_->HandleTouchEvent(touch_event);
      }
      break;
    }
    case LynxEventType::kCustom: {
      ui_owner_->HandleCustomEvent(static_cast<const CustomEvent&>(event));
      break;
    }
    case LynxEventType::kMouse: {
      // TODO(hexionghui): handle mouse event
      break;
    }
    case LynxEventType::kWheel: {
      // TODO(hexionghui): handle wheel event
      break;
    }
    case LynxEventType::kKeyboard: {
      // TODO(hexionghui): handle keyboard event
      break;
    }
    case LynxEventType::kNone:
    default: {
      break;
    }
  }
}

void EventEmitter::SendPseudoStatusEvent(int id, PseudoStatus pre_status,
                                         PseudoStatus current_status) const {
  if (pre_status == current_status) {
    return;
  }
  ui_owner_->OnPseudoStatusChanged(id, pre_status, current_status);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
