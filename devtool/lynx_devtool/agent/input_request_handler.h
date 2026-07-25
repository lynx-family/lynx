// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

class DevToolPlatformFacade;
class LynxDevToolMediator;

namespace input {
class SyntheticGestureController;
}  // namespace input

// Handles CDP Input domain requests on the UI thread.
class InputRequestHandler {
 public:
  explicit InputRequestHandler(
      const std::weak_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InputRequestHandler();

  // Updates the platform facade synchronously and returns true when it
  // changed. The caller must run Reset() on the UI thread in that case, because
  // the gesture controller is UI-thread-bound.
  bool SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade);
  // Releases the gesture controller. Must run on the UI thread.
  void Reset();

  void EmulateTouchFromMouseEvent(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message);
  void InsertText(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message);
  void SynthesizeTapGesture(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message);

 private:
  void EnsureSyntheticGestureController(
      const fml::RefPtr<fml::TaskRunner>& task_runner);

  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::shared_ptr<DevToolPlatformFacade> devtool_platform_facade_;
  std::shared_ptr<input::SyntheticGestureController>
      synthetic_gesture_controller_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INPUT_REQUEST_HANDLER_H_
