// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_network_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

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
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto it = functions_map_.find(method);
  if (it == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(it->second))(responder, message["params"]);
}

void InspectorNetworkAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NetworkEnable(responder, params);
}

void InspectorNetworkAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NetworkDisable(responder, params);
}

void InspectorNetworkAgent::GetResponseBody(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NetworkGetResponseBody(responder, params);
}

void InspectorNetworkAgent::GetRequestPostData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NetworkGetRequestPostData(responder, params);
}

}  // namespace devtool
}  // namespace lynx
