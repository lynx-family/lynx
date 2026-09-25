// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_component_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorComponentAgent::InspectorComponentAgent()
    : InspectorComponentAgent(LynxGlobalDevToolMediator::GetInstance()) {}

InspectorComponentAgent::InspectorComponentAgent(
    const LynxGlobalDevToolMediator& global_devtool_mediator)
    : global_devtool_mediator_(global_devtool_mediator) {
  functions_map_["Component.uselessUpdate"] =
      &InspectorComponentAgent::UselessUpdate;
}

void InspectorComponentAgent::UselessUpdate(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  Json::Value event(Json::objectValue);
  event["method"] = "Component.uselessUpdate";
  event["params"] = params;
  global_devtool_mediator_.SendCDPEvent(event);
  responder->SendSuccess();
}

void InspectorComponentAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}

}  // namespace devtool
}  // namespace lynx
