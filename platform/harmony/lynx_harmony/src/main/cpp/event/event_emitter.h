// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_EMITTER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_EMITTER_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxEvent;
class TouchEvent;
class CustomEvent;
class UIOwner;

class EventEmitter {
 public:
  explicit EventEmitter(UIOwner* ui_owner) : ui_owner_(ui_owner) {}

  void SendEvent(const LynxEvent& event) const;

  void SendPseudoStatusEvent(int id, PseudoStatus pre_status,
                             PseudoStatus current_status) const;

 private:
  UIOwner* ui_owner_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_EVENT_EMITTER_H_
