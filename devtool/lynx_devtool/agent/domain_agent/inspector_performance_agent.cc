// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_performance_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"

namespace lynx {
namespace devtool {

InspectorPerformanceAgent::InspectorPerformanceAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Performance.enable"] = &InspectorPerformanceAgent::Enable;
  functions_map_["Performance.disable"] = &InspectorPerformanceAgent::Disable;
  functions_map_["Performance.getAllTimingInfo"] =
      &InspectorPerformanceAgent::getAllTimingInfo;
  functions_map_["Performance.getAllPerformanceEntries"] =
      &InspectorPerformanceAgent::getAllPerformanceEntries;
}

InspectorPerformanceAgent::~InspectorPerformanceAgent() = default;

void InspectorPerformanceAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PerformanceEnable(responder, params);
}

void InspectorPerformanceAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->PerformanceDisable(responder, params);
}

void InspectorPerformanceAgent::getAllTimingInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->getAllTimingInfo(responder, params);
}

void InspectorPerformanceAgent::getAllPerformanceEntries(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->getAllPerformanceEntries(responder, params);
}

void InspectorPerformanceAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(responder, message["params"]);
  } else {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  }
}

}  // namespace devtool
}  // namespace lynx
