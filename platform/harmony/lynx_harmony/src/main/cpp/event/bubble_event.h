// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_BUBBLE_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_BUBBLE_EVENT_H_

#include <string>
#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/lynx_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BubbleEvent : public LynxEvent {
 public:
  BubbleEvent(int id, std::string name, LynxEventType type, lepus::Value params)
      : LynxEvent(id, std::move(name), type), params_(std::move(params)) {}

  const lepus::Value& Params() const { return params_; }

 private:
  lepus::Value params_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_BUBBLE_EVENT_H_
