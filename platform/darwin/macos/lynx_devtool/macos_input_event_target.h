// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_MACOS_INPUT_EVENT_TARGET_H_
#define PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_MACOS_INPUT_EVENT_TARGET_H_

#include <memory>

#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {

// Main-thread-only native mouse injection. Coordinates are points relative to
// the top-left of the containing window's content view, including native views.
class MacOSInputEventTarget final : public input::InputEventTarget {
 public:
  // host_view is an NSView*, held weakly.
  explicit MacOSInputEventTarget(void* host_view);
  ~MacOSInputEventTarget() override;

  input::PointerCapabilities GetPointerCapabilities() const override;
  bool InjectPointerEvent(const input::PointerEvent& event) override;

  // Cancel before reparenting or resetting the native view.
  void CancelActivePointer();
  // Release any pressed mouse before the renderer removes its native view.
  void Invalidate();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_DARWIN_MACOS_LYNX_DEVTOOL_MACOS_INPUT_EVENT_TARGET_H_
