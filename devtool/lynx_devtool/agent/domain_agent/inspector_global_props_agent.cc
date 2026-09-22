// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_global_props_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"

namespace lynx {
namespace devtool {

InspectorGlobalPropsAgent::InspectorGlobalPropsAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["GlobalProps.enable"] = &InspectorGlobalPropsAgent::Enable;
  functions_map_["GlobalProps.disable"] = &InspectorGlobalPropsAgent::Disable;
  functions_map_["GlobalProps.get"] = &InspectorGlobalPropsAgent::Get;
  functions_map_["GlobalProps.replace"] = &InspectorGlobalPropsAgent::Replace;
}

void InspectorGlobalPropsAgent::CallMethod(
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

void InspectorGlobalPropsAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GlobalPropsEnable(responder, params);
}

void InspectorGlobalPropsAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GlobalPropsDisable(responder, params);
}

void InspectorGlobalPropsAgent::Get(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GlobalPropsGet(responder, params);
}

void InspectorGlobalPropsAgent::Replace(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GlobalPropsReplace(responder, params);
}

}  // namespace devtool
}  // namespace lynx
