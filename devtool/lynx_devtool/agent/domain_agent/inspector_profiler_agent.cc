// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_profiler_agent.h"

namespace lynx {
namespace devtool {
InspectorProfilerAgent::InspectorProfilerAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {}
void InspectorProfilerAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->DispatchJSMessage(message);
}

}  // namespace devtool
}  // namespace lynx
