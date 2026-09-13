// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_log_agent.h"

namespace lynx {
namespace devtool {

InspectorLogAgent::InspectorLogAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Log.enable"] = &InspectorLogAgent::Enable;
  functions_map_["Log.disable"] = &InspectorLogAgent::Disable;
  functions_map_["Log.clear"] = &InspectorLogAgent::Clear;
}

InspectorLogAgent::~InspectorLogAgent() = default;

void InspectorLogAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  std::map<std::string, LogAgentMethod>::iterator iter;
  // Do not process Log messages for the MTS target to avoid sending duplicate
  // `Log.entryAdded` messages.
  if (message.isMember("sessionId") ||
      (iter = functions_map_.find(method)) == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

void InspectorLogAgent::Enable(const std::shared_ptr<CDPResponder>& responder,
                               const Json::Value& params) {
  devtool_mediator_->LogEnable(responder, params);
}

void InspectorLogAgent::Disable(const std::shared_ptr<CDPResponder>& responder,
                                const Json::Value& params) {
  devtool_mediator_->LogDisable(responder, params);
}

void InspectorLogAgent::Clear(const std::shared_ptr<CDPResponder>& responder,
                              const Json::Value& params) {
  devtool_mediator_->LogClear(responder, params);
}

}  // namespace devtool
}  // namespace lynx
