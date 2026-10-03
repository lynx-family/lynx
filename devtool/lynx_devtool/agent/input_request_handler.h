// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class DevToolPlatformFacade;
class LynxDevToolMediator;

namespace input {
class InputEventTarget;
class SyntheticGestureController;
}  // namespace input

// Handles CDP Input domain requests on the UI thread.
class InputRequestHandler {
 public:
  explicit InputRequestHandler(
      const std::weak_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InputRequestHandler();

  // Updates the facade synchronously and returns true when it or its input
  // target changed. The caller must run Reset() on the UI thread in that case,
  // because the gesture controller is UI-thread-bound.
  bool SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade);
  // Releases the gesture controller. Must run on the UI thread.
  void Reset();

  DECLARE_DEVTOOL_CDP_METHOD(EmulateTouchFromMouseEvent);
  DECLARE_DEVTOOL_CDP_METHOD(DispatchMouseEvent);
  DECLARE_DEVTOOL_CDP_METHOD(InsertText);
  DECLARE_DEVTOOL_CDP_METHOD(SynthesizeTapGesture);

 private:
  void EnsureSyntheticGestureController(
      const fml::RefPtr<fml::TaskRunner>& task_runner);

  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::shared_ptr<DevToolPlatformFacade> devtool_platform_facade_;
  std::weak_ptr<input::InputEventTarget> input_event_target_wp_;
  std::shared_ptr<input::SyntheticGestureController>
      synthetic_gesture_controller_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_
