// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_native_module_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"

namespace lynx {
namespace devtool {

InspectorLynxNativeModuleAgent::InspectorLynxNativeModuleAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["LynxNativeModule.enable"] =
      &InspectorLynxNativeModuleAgent::Enable;
  functions_map_["LynxNativeModule.disable"] =
      &InspectorLynxNativeModuleAgent::Disable;
  functions_map_["LynxNativeModule.getRecords"] =
      &InspectorLynxNativeModuleAgent::GetRecords;
}

InspectorLynxNativeModuleAgent::~InspectorLynxNativeModuleAgent() = default;

void InspectorLynxNativeModuleAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

void InspectorLynxNativeModuleAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NativeModuleEnable(responder, params);
}

void InspectorLynxNativeModuleAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NativeModuleDisable(responder, params);
}

void InspectorLynxNativeModuleAgent::GetRecords(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->NativeModuleGetRecords(responder, params);
}

}  // namespace devtool
}  // namespace lynx
