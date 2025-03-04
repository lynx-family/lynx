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

void InspectorLogAgent::CallMethod(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorLogAgent::Enable(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message) {
  devtool_mediator_->LogEnable(sender, message);
}

void InspectorLogAgent::Disable(const std::shared_ptr<MessageSender>& sender,
                                const Json::Value& message) {
  devtool_mediator_->LogDisable(sender, message);
}

void InspectorLogAgent::Clear(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message) {
  devtool_mediator_->LogClear(sender, message);
}

void InspectorLogAgent::SendLog(const std::shared_ptr<MessageSender>& sender,
                                const lynx::piper::ConsoleMessage& message) {
  devtool_mediator_->SendLogEntryAddedEvent(message);
}

}  // namespace devtool
}  // namespace lynx
