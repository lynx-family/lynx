// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_debugger_agent.h"

namespace lynx {
namespace devtool {

InspectorDebuggerAgent::InspectorDebuggerAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Debugger.enable"] = &InspectorDebuggerAgent::Enable;
}

void InspectorDebuggerAgent::Enable(const Json::Value& message) {
  if (!message.isMember("sessionId")) {
    devtool_mediator_->UpdateTarget();
  }
}

void InspectorDebuggerAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  // The JavaScript inspector emits the complete CDP response asynchronously.
  // Release responder ownership so it does not send a fallback response.
  (void)responder->RetrieveSender();
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(message);
  }
  devtool_mediator_->DispatchJSMessage(message);
}
}  // namespace devtool
}  // namespace lynx
