// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_performance_agent.h"

namespace lynx {
namespace devtool {

InspectorPerformanceAgent::InspectorPerformanceAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Performance.enable"] = &InspectorPerformanceAgent::Enable;
  functions_map_["Performance.disable"] = &InspectorPerformanceAgent::Disable;
  functions_map_["Performance.getAllTimingInfo"] =
      &InspectorPerformanceAgent::getAllTimingInfo;
}

InspectorPerformanceAgent::~InspectorPerformanceAgent() = default;

void InspectorPerformanceAgent::Enable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PerformanceEnable(sender, message);
}

void InspectorPerformanceAgent::Disable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PerformanceDisable(sender, message);
}

void InspectorPerformanceAgent::getAllTimingInfo(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->getAllTimingInfo(sender, message);
}

void InspectorPerformanceAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& content) {
  std::string method = content["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(sender, content);
  } else {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = content["id"].asInt64();
    sender->SendMessage("CDP", res);
  }
}

}  // namespace devtool
}  // namespace lynx
