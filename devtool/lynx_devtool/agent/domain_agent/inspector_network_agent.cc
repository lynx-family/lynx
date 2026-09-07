// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_network_agent.h"

namespace lynx {
namespace devtool {

InspectorNetworkAgent::InspectorNetworkAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Network.enable"] = &InspectorNetworkAgent::Enable;
  functions_map_["Network.disable"] = &InspectorNetworkAgent::Disable;
  functions_map_["Network.getResponseBody"] =
      &InspectorNetworkAgent::GetResponseBody;
  functions_map_["Network.getRequestPostData"] =
      &InspectorNetworkAgent::GetRequestPostData;
}

void InspectorNetworkAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto it = functions_map_.find(method);
  if (it == functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
    return;
  }
  (this->*(it->second))(sender, message);
}

void InspectorNetworkAgent::Enable(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message) {
  devtool_mediator_->NetworkEnable(sender, message);
}

void InspectorNetworkAgent::Disable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->NetworkDisable(sender, message);
}

void InspectorNetworkAgent::GetResponseBody(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->NetworkGetResponseBody(sender, message);
}

void InspectorNetworkAgent::GetRequestPostData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->NetworkGetRequestPostData(sender, message);
}

}  // namespace devtool
}  // namespace lynx
