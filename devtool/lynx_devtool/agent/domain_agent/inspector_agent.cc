// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_agent.h"

namespace lynx {
namespace devtool {

InspectorAgent::InspectorAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Inspector.enable"] = &InspectorAgent::Enable;
  functions_map_["Inspector.detached"] = &InspectorAgent::Detached;
}

InspectorAgent::~InspectorAgent() = default;

void InspectorAgent::Enable(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message) {
  devtool_mediator_->InspectorEnable(sender, message);
}

void InspectorAgent::Detached(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message) {
  devtool_mediator_->InspectorDetached(sender, message);
}

void InspectorAgent::CallMethod(const std::shared_ptr<MessageSender>& sender,
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
}  // namespace devtool
}  // namespace lynx
