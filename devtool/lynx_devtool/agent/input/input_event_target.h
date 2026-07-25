// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_INPUT_EVENT_TARGET_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_INPUT_EVENT_TARGET_H_

#include <functional>

#include "devtool/lynx_devtool/agent/input/input_event.h"

namespace lynx {
namespace input {

class InputEventTarget {
 public:
  virtual ~InputEventTarget() = default;

  virtual PointerCapabilities GetPointerCapabilities() const = 0;
  virtual bool InjectPointerEvent(const PointerEvent& event) = 0;

  // The callback must run on the same sequence that called this method.
  virtual void WaitForInputProcessed(std::function<void(bool)> callback) {
    callback(true);
  }
};

}  // namespace input
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_INPUT_EVENT_TARGET_H_
